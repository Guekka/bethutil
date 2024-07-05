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

void ModFolder::iterate(const std::function<void(Path relative_path)> &loose,
                        const std::function<void(const Path &archive_path)> &archive) noexcept
{
    auto is_arch = [](const Path &file_name) {
        const auto ext = common::to_lower(file_name.extension().u8string());
        return common::contains(bsa::k_archive_extensions, ext);
    };

    auto files = flux::from_range(fs::recursive_directory_iterator(dir_))
                     .filter([](auto &&e) { return e.is_regular_file(); })
                     .map([](auto &&e) { return e.path(); })
                     .to<std::vector>();

    const auto fut = thread_pool_.submit_loop(size_t{0},
                                              files.size(),
                                              [&is_arch, this, &archive, &loose, &files](auto idx) {
                                                  auto &&path = files[idx];
                                                  if (!is_arch(path)) [[likely]]
                                                      loose(path.lexically_relative(dir_));

                                                  else if (is_arch(path)) [[unlikely]]
                                                      archive(path);
                                              });

    fut.wait();
    // TODO: there might be an exception in fut. Should we ignore it?
}

auto ModFolder::size() noexcept -> size_t
{
    std::atomic<size_t> size = 0;
    iterate([&size](const Path &) { size += 1; },
            [&size](const Path &archive_path) {
                if (const auto arch = bsa::Archive::read(archive_path); arch)
                    size += arch->size();
            });

    return size;
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

void ModFolder::transform(ModFolderTransformer &transformer) noexcept
{
    const auto transform_loose = [this, &transformer](const Path &relative_path) {
        const auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, common::Error>>(
            [&relative_path, this] { return common::read_file(dir_ / relative_path); });

        if (const auto transformed = transformer.transform_file({relative_path, file_data}))
        {
            if (!common::write_file(dir_ / relative_path, *transformed))
                transformer.failed_to_write_transformed_file(relative_path, *transformed);
        }
    };

    const auto transform_archive = [&transformer, this](const Path &archive_path) {
        if (const auto arch_size = file_size(archive_path); arch_size > bsa_settings_.max_size)
        {
            if (const auto action = transformer.archive_too_large(archive_path, BeforeProcessing);
                action == Skip)
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
            auto fut = thread_pool_.submit_task([&transformer, &any_file_changed, &pair] {
                auto &[relative_path, file] = pair;

                auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, common::Error>>(
                    [&pair]() -> tl::expected<std::vector<std::byte>, common::Error> {
                        auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};
                        if (!pair.second.write(buffer))
                            // TODO: better error here?
                            return tl::make_unexpected(
                                common::Error(std::error_code(errno, std::system_category())));

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
            });
            futs.push_back(std::move(fut));
        }
        flux::for_each(futs, [](auto &&fut) { fut.wait(); });
        // TODO: there might be an exception in fut. Should we ignore it?

        const auto target_version = guess_target_archive_version(archive, bsa_settings_);
        if (target_version)
            archive.set_version(*target_version);

        if (const auto arch_size = archive.file_size(); arch_size > bsa_settings_.max_size)
        {
            if (const auto action = transformer.archive_too_large(archive_path, AfterProcessing);
                action == Skip)
                return;
        }

        if (any_file_changed || target_version)
        {
            // Change the extension of the archive if needed
            auto path = archive_path;
            if (path.extension() != bsa_settings_.extension)
                path.replace_extension(bsa_settings_.extension);

            if (!std::move(archive).write(path))
                transformer.failed_to_write_archive(archive_path, path);

            // Remove the old archive if the new one has a different name
            if (path != archive_path)
                fs::remove(archive_path);
        }
    };

    iterate(transform_loose, transform_archive);
};
} // namespace btu::modmanager
