/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>
#include <btu/common/functional.hpp>
#include <flux.hpp>

#include <atomic>
#include <utility>

namespace btu::modmanager {

ModFolder::ModFolder(Path directory, bsa::Settings bsa_settings)
    : dir_(std::move(directory))
    , bsa_settings_(BTU_MOV(bsa_settings))
{
}

void ModFolder::iterate(
    const std::function<void(Path relative_path)> &loose,
    const std::function<void(const Path &archive_path, bsa::Archive &&archive)> &archive) const noexcept
{
    auto is_arch = [](const Path &file_name) {
        const auto ext = common::to_lower(file_name.extension().u8string());
        return common::contains(bsa::k_archive_extensions, ext);
    };

    auto files =

        flux::from_range(fs::recursive_directory_iterator(dir_))
            .filter([](auto &&e) { return e.is_regular_file(); })
            .map([](auto &&e) { return e.path(); })
            .to<std::vector>();

    common::for_each_mt(files, [&is_arch, this, &archive, &loose](auto &&path) {
        if (!is_arch(path)) [[likely]]
        {
            loose(path.lexically_relative(dir_));
            return;
        }

        if (auto arch = bsa::Archive::read(path))
            archive(path, std::move(*arch));
    });
}

auto ModFolder::size() const noexcept -> size_t
{
    std::atomic<size_t> size = 0;
    iterate([&size](const Path &) { size += 1; },
            [&size](const Path &, const bsa::Archive &archive) { size += archive.size(); });

    return size;
}

void ModFolder::iterate(const std::function<void(ModFile)> &visitor,
                        ArchiveTooLargeHandler &&archive_too_large_handler) const noexcept
{
    transform_impl(
        [&visitor](ModFile file) {
            visitor(std::move(file));
            return std::nullopt;
        },
        std::move(archive_too_large_handler));
}

void ModFolder::transform(const Transformer &transformer,
                          ArchiveTooLargeHandler &&archive_too_large_handler) noexcept
{
    transform_impl(transformer, std::move(archive_too_large_handler));
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

void ModFolder::transform_impl(const Transformer &transformer,
                               ArchiveTooLargeHandler &&archive_too_large_handler) const noexcept
{
    iterate(
        [this, &transformer](const Path &relative_path) {
            const auto file_data = common::Lazy<tl::expected<std::vector<std::byte>, btu::common::Error>>(
                [&relative_path, this] { return common::read_file(dir_ / relative_path); });

            if (const auto transformed = transformer({relative_path, file_data}))
            {
                const auto res = common::write_file(dir_ / relative_path, *transformed);
                if (!res)
                {
                    // TODO: what should we do here? We're not going to cancel the whole operation just
                    //  because of one file that failed to write, right?
                }
            }
        },
        [&transformer,
         this,
         archive_too_large_handler = std::move(archive_too_large_handler)](const Path &archive_path,
                                                                           bsa::Archive &&archive) {
            // Check if the archive is too large and the caller wants to skip it
            auto check_archive_and_skip = [&](ArchiveTooLargeState state) {
                if (archive.file_size() > bsa_settings_.max_size)
                    return archive_too_large_handler(archive_path, state) == ArchiveTooLargeAction::Skip;
                return false;
            };

            if (check_archive_and_skip(ArchiveTooLargeState::BeforeProcessing))
                return;

            std::atomic_bool any_file_changed = false;

            common::for_each_mt(archive, [transformer, &any_file_changed](auto &pair) {
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

                auto transformed = transformer({relative_path, BTU_MOV(file_data)});
                if (transformed)
                {
                    const bool res = file.read(*transformed);
                    if (!res)
                    {
                        // TODO: how could we handle this?
                    }
                    any_file_changed = any_file_changed || res;
                }
            });

            const auto target_version = guess_target_archive_version(archive, bsa_settings_);
            if (target_version)
                archive.set_version(*target_version);

            if (check_archive_and_skip(ArchiveTooLargeState::AfterProcessing))
                return;

            if (any_file_changed || target_version)
            {
                // Change the extension of the archive if needed
                auto path = archive_path;
                if (path.extension() != bsa_settings_.extension)
                    path.replace_extension(bsa_settings_.extension);

                // TODO: replace throw with a better error handling mechanism
                if (!std::move(archive).write(path))
                    throw std::runtime_error(std::string("Failed to write archive: ")
                                             + common::as_ascii_string(path.u8string()));

                // Remove the old archive if the new one has a different name
                if (path != archive_path)
                    fs::remove(archive_path);
            }
        });
}

} // namespace btu::modmanager
