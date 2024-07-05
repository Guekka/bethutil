/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>
#include <flux.hpp>

#include <atomic>
#include <utility>

namespace btu::modmanager {

ModFolder::ModFolder(Path directory, bsa::Settings bsa_settings)
    : dir_(std::move(directory))
    , bsa_settings_(BTU_MOV(bsa_settings))
    , thread_pool_(common::make_thread_pool())
{
}

auto ModFolder::size() noexcept -> size_t
{
    class SizeIterator final : public ModFolderIterator
    {
    public:
        size_t size;

        [[nodiscard]] auto archive_too_large(const Path & /*archive_path*/,
                                             ArchiveTooLargeState /*state*/) noexcept
            -> ArchiveTooLargeAction override
        {
            return Skip;
        }

        void process_file(ModFile /*file*/) noexcept override { size += 1; }
    };

    auto iterator = SizeIterator{};
    iterate(iterator);
    return iterator.size;
}

void ModFolder::iterate(ModFolderIterator &iterator) noexcept
{
    // creates a ModFolderTransformer based on the user-provided ModFolderIterator
    class ModFolderProcessorReadOnly final : public ModFolderTransformer
    {
    public:
        explicit ModFolderProcessorReadOnly(ModFolderIterator &iterator) noexcept
            : iterator_(iterator)
        {
        }

        [[nodiscard]] auto archive_too_large(const Path &archive_path, ArchiveTooLargeState state) noexcept
            -> ArchiveTooLargeAction override
        {
            return iterator_.get().archive_too_large(archive_path, state);
        }

        [[nodiscard]] auto transform_file(ModFile file) noexcept
            -> std::optional<std::vector<std::byte>> override
        {
            iterator_.get().process_file(file);
            return std::nullopt;
        }

        [[nodiscard]] auto stop_requested() const noexcept -> bool override
        {
            return iterator_.get().stop_requested();
        }

    private:
        std::reference_wrapper<ModFolderIterator> iterator_;
    } transformer(iterator);

    transform(transformer);
}

/**
 * \brief Guesses the target archive version of the given archive based on the provided settings.
 *
 * \param archive The archive to guess the target version for.
 * \param bsa_settings The settings to be used for guessing the version.
 *
 * \return An optional `bsa::ArchiveVersion` indicating the guessed target version
 *         or `std::nullopt` if the target version could not be determined. This can also happen if the
 *         target version is the same as the current version.
 *
 * \see bsa::Archive
 * \see bsa::Settings
 */
[[nodiscard]] auto guess_target_archive_version(const bsa::Archive &archive,
                                                const bsa::Settings &bsa_settings) noexcept
    -> std::optional<bsa::ArchiveVersion>
{
    const auto target = bsa_settings.version;

    if (target == archive.version())
        return std::nullopt;

    return target;
}

/// Multiple users reported freezing. This should let other threads run.
/// There is probably a better way, but I don't know it
void reduce_cpu_usage() noexcept
{
    std::this_thread::sleep_for(std::chrono::nanoseconds(5));
}

void transform_loose_file(const Path &absolute_path,
                          const Path &dir,
                          ModFolderTransformer &transformer) noexcept
{
    if (transformer.stop_requested())
        return;

    reduce_cpu_usage();

    const auto relative_path = absolute_path.lexically_relative(dir);

    const auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, common::Error>>(
        [&absolute_path] { return common::read_file(absolute_path); });

    if (const auto transformed = transformer.transform_file({relative_path, file_data}))
    {
        if (!common::write_file(absolute_path, *transformed))
            transformer.failed_to_write_transformed_file(relative_path, *transformed);
    }
}

[[nodiscard]] auto want_to_skip_archive(const Path &archive_path,
                                        ModFolderTransformer &transformer,
                                        ModFolderIteratorBase::ArchiveTooLargeState state) noexcept -> bool
{
    return transformer.archive_too_large(archive_path, state)
           == ModFolderIteratorBase::ArchiveTooLargeAction::Skip;
}

[[nodiscard]] auto transform_archive_file_inner(ModFolderTransformer &transformer,
                                                std::atomic_bool &any_file_changed,
                                                bsa::Archive::value_type &pair) noexcept
{
    return [&transformer, &any_file_changed, &pair] {
        if (transformer.stop_requested())
            return;

        reduce_cpu_usage();

        auto &[relative_path, file] = pair;

        auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, common::Error>>(
            [&pair]() -> tl::expected<std::vector<std::byte>, common::Error> {
                auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};
                if (!pair.second.write(buffer))
                    // TODO: better error here?
                    return tl::make_unexpected(common::Error(std::error_code(errno, std::system_category())));

                return buffer.get<binary_io::memory_ostream>().rdbuf();
            });

        auto transformed = transformer.transform_file({relative_path, file_data});
        if (transformed)
        {
            const bool res = file.read(*transformed);
            if (!res)
                transformer.failed_to_read_transformed_file(relative_path, *transformed);

            any_file_changed = any_file_changed || res;
        }
    };
}

[[nodiscard]] auto change_archive_version_if_needed(bsa::Archive &archive,
                                                    const bsa::Settings &bsa_settings) noexcept -> bool
{
    if (const auto target_version = guess_target_archive_version(archive, bsa_settings))
    {
        archive.set_version(*target_version);
        return true;
    }
    return false;
}

void transform_archive_file(const Path &archive_path,
                            ModFolderTransformer &transformer,
                            const bsa::Settings &bsa_settings,
                            common::ThreadPool &thread_pool) noexcept
{
    if (const auto arch_size = file_size(archive_path); arch_size > bsa_settings.max_size)
    {
        if (want_to_skip_archive(archive_path,
                                 transformer,
                                 ModFolderIteratorBase::ArchiveTooLargeState::BeforeProcessing))
            return;
    }

    auto opt_arch = bsa::Archive::read(archive_path);
    if (!opt_arch)
    {
        transformer.failed_to_read_archive(archive_path);
        return;
    }

    auto archive                      = std::move(*opt_arch);
    std::atomic_bool any_file_changed = false;
    std::vector<std::future<void>> futs;
    for (auto &pair : archive)
    {
        if (transformer.stop_requested())
            return;

        auto fut = thread_pool.submit_task(transform_archive_file_inner(transformer, any_file_changed, pair));
        futs.push_back(std::move(fut));
    }
    flux::for_each(futs, [](auto &&fut) { fut.wait(); });
    // TODO: there might be an exception in fut. Should we ignore it?

    if (transformer.stop_requested())
        return;

    const bool version_changed = change_archive_version_if_needed(archive, bsa_settings);

    if (const auto arch_size = archive.file_size(); arch_size > bsa_settings.max_size)
    {
        if (want_to_skip_archive(archive_path,
                                 transformer,
                                 ModFolderIteratorBase::ArchiveTooLargeState::AfterProcessing))
            return;
    }

    if (transformer.stop_requested())
        return;

    if (any_file_changed || version_changed)
    {
        // Change the extension of the archive if needed
        auto path = archive_path;
        if (path.extension() != bsa_settings.extension)
            path.replace_extension(bsa_settings.extension);

        if (!std::move(archive).write(path))
        {
            transformer.failed_to_write_archive(archive_path, path);
            return;
        }

        // Remove the old archive if the new one has a different name
        if (!equivalent(archive_path, path))
            fs::remove(archive_path);
    }
}

void ModFolder::transform(ModFolderTransformer &transformer) noexcept
{
    auto is_arch = [](const Path &file_name) {
        const auto ext = common::to_lower(file_name.extension().u8string());
        return common::contains(bsa::k_archive_extensions, ext);
    };

    auto files = flux::from_range(fs::recursive_directory_iterator(dir_))
                     .filter([](auto &&e) { return e.is_regular_file(); })
                     .map([](auto &&e) { return e.path(); })
                     .to<std::vector>();

    std::vector<std::future<void>> futs;
    for (const auto &file_path : files)
    {
        if (transformer.stop_requested())
            return;

        futs.push_back(thread_pool_.submit_task([this, &file_path, &is_arch, &transformer] {
            if (is_arch(file_path)) [[unlikely]]
                transform_archive_file(file_path, transformer, bsa_settings_, thread_pool_);
            else [[likely]]
                transform_loose_file(file_path, dir_, transformer);
        }));
    }
    flux::for_each(futs, [](auto &&fut) { fut.wait(); });
    // TODO: there might be an exception in fut. Should we ignore it?
};
} // namespace btu::modmanager
