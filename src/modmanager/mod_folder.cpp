/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>
#include <flow.hpp>

#include <atomic>
#include <execution>
#include <utility>

namespace btu::modmanager {

ModFolder::ModFolder(Path directory, btu::bsa::Settings bsa_settings)
    : dir_(std::move(directory))
    , bsa_settings_(BTU_MOV(bsa_settings))
{
}

void ModFolder::iterate(
    const std::function<void(Path relative_path)> &loose,
    const std::function<void(const Path &archive_path, bsa::Archive &&archive)> &archive) const
{
    auto is_arch = [](const Path &file_name) {
        auto ext = btu::common::to_lower(file_name.extension().u8string());
        return btu::common::contains(btu::bsa::k_archive_extensions, ext);
    };

    auto files =

        flow::from(fs::recursive_directory_iterator(dir_))
            .filter([](auto &&e) { return e.is_regular_file(); })
            .map([](auto &&e) { return e.path(); })
            .to_vector();

    std::for_each(std::execution::par,
                  files.begin(),
                  files.end(),
                  [&is_arch, this, &archive, &loose](auto &&path) {
                      if (!is_arch(path)) [[likely]]
                      {
                          loose(path.lexically_relative(dir_));
                          return;
                      }

                      if (auto arch = btu::bsa::read_archive(path))
                          archive(path, std::move(*arch));
                  });
}

auto ModFolder::size() const -> size_t
{
    std::atomic<size_t> size = 0;
    iterate([&size](const Path &) { size += 1; },
            [&size](const Path &, bsa::Archive &&archive) { size += archive.size(); });

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

void ModFolder::transform(ModFolder::Transformer &&transformer,
                          ArchiveTooLargeHandler &&archive_too_large_handler)
{
    transform_impl(std::move(transformer), std::move(archive_too_large_handler));
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
    auto version = archive_version(archive);
    if (!version)
        return std::nullopt;

    auto type = archive_type(archive);
    if (!type)
        return std::nullopt;

    auto target = [type, &bsa_settings]() -> std::optional<bsa::ArchiveVersion> {
        switch (*type)
        {
            case bsa::ArchiveType::Textures:
                return bsa_settings.texture_version.value_or(bsa_settings.version);
            case bsa::ArchiveType::Standard: return bsa_settings.version;
        }
        return std::nullopt;
    }();

    if (target == *version)
        return std::nullopt;

    return target;
}

void ModFolder::transform_impl(ModFolder::Transformer &&transformer,
                               ArchiveTooLargeHandler &&archive_too_large_handler) const
{
    iterate(
        [this, &transformer](const Path &relative_path) {
            auto file_data   = common::read_file(dir_ / relative_path);
            auto transformed = transformer({relative_path, file_data});
            if (transformed)
                common::write_file(dir_ / relative_path, *transformed);
        },
        [&transformer, this, &archive_too_large_handler](const Path &archive_path, bsa::Archive &&archive) {
            // Check if the archive is too large and the caller wants to skip it
            auto check_archive_and_skip = [&](ArchiveTooLargeState state) {
                if (bsa::archive_size(archive) > bsa_settings_.max_size)
                    return archive_too_large_handler(archive_path, state) == ArchiveTooLargeAction::Skip;
                return false;
            };

            if (check_archive_and_skip(ArchiveTooLargeState::BeforeProcessing))
                return;

            bool any_file_changed = false;

            for (auto &[relative_path, file] : archive)
            {
                auto buffer = binary_io::any_ostream{binary_io::memory_ostream{}};
                file.write(buffer);

                auto &file_data = buffer.get<binary_io::memory_ostream>().rdbuf();

                auto transformed = transformer({relative_path, BTU_MOV(file_data)});

                if (transformed)
                {
                    file.read(*transformed);
                    any_file_changed = true;
                }
            }

            const auto target_version = guess_target_archive_version(archive, bsa_settings_);
            if (target_version)
                bsa::set_archive_version(archive, *target_version);

            if (check_archive_and_skip(ArchiveTooLargeState::AfterProcessing))
                return;

            if (any_file_changed || target_version)
            {
                // Change the extension of the archive if needed
                auto path = archive_path;
                if (path.extension() != bsa_settings_.extension)
                    path.replace_extension(bsa_settings_.extension);

                write_archive(std::move(archive), path);

                // Remove the old archive if the new one has a different name
                if (path != archive_path)
                    fs::remove(archive_path);
            }
        });
}

} // namespace btu::modmanager
