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
    const std::function<void(const Path &archive_path, bsa::Archive &&archive)> &archive) const
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

auto ModFolder::size() const -> size_t
{
    std::atomic<size_t> size = 0;
    iterate([&size](const Path &) { size += 1; },
            [&size](const Path &, const bsa::Archive &archive) { size += archive.size(); });

    return size;
}

void ModFolder::iterate(const std::function<void(ModFile)> &visitor,
                        ArchiveTooLargeHandler &&archive_too_large_handler) const
{
    transform_impl(
        [&visitor](ModFile file) {
            visitor(std::move(file));
            return std::nullopt;
        },
        std::move(archive_too_large_handler));
}

void ModFolder::transform(const Transformer &transformer, ArchiveTooLargeHandler &&archive_too_large_handler)
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
 * \note This function does not throw any exceptions.
 *
 * \see bsa::Archive
 * \see bsa::Settings
 */
[[nodiscard]] auto guess_target_archive_version(const bsa::Archive &archive,
                                                const bsa::Settings bsa_settings) noexcept
    -> std::optional<bsa::ArchiveVersion>
{
    const auto version = archive.version();
    auto type          = archive.type();

    const auto target = [type, &bsa_settings]() -> std::optional<bsa::ArchiveVersion> {
        switch (type)
        {
            case bsa::ArchiveType::Textures:
                return bsa_settings.texture_version.value_or(bsa_settings.version);
            case bsa::ArchiveType::Standard: return bsa_settings.version;
        }
        return std::nullopt;
    }();

    if (target == version)
        return std::nullopt;

    return target;
}

void ModFolder::transform_impl(const Transformer &transformer,
                               ArchiveTooLargeHandler &&archive_too_large_handler) const
{
    iterate(
        [this, &transformer](const Path &relative_path) {
            const auto file_data = common::Lazy<std::vector<std::byte>>(
                [&relative_path, this] { return common::read_file(dir_ / relative_path); });

            if (const auto transformed = transformer({relative_path, file_data}))
                common::write_file(dir_ / relative_path, *transformed);
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

            bool any_file_changed = false;

            common::for_each_mt(archive, [transformer, &any_file_changed](auto &pair) {
                auto &[relative_path, file] = pair;

                auto file_data = common::Lazy<std::vector<std::byte>>([&pair] {
                    auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};
                    pair.second.write(buffer);
                    return buffer.get<binary_io::memory_ostream>().rdbuf();
                });

                auto transformed = transformer({relative_path, BTU_MOV(file_data)});
                if (transformed)
                {
                    file.read(*transformed);
                    any_file_changed = true;
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

                std::move(archive).write(path);

                // Remove the old archive if the new one has a different name
                if (path != archive_path)
                    fs::remove(archive_path);
            }
        });
}

} // namespace btu::modmanager
