/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/error_code.hpp"

#include <btu/common/string.hpp>
#include <btu/common/threading.hpp>
#include <tl/expected.hpp>

#include <unordered_set>

namespace btu::bsa {
namespace {
[[nodiscard]] auto lower_generic_path(const Path &path) -> std::u8string
{
    return common::to_lower(path.generic_u8string());
}

[[nodiscard]] auto make_unpack_preview(Archive &archive, const UnpackSettings &settings) -> UnpackPreview
{
    const auto root = settings.extract_to_dir.value_or(settings.file_path.parent_path());

    auto relative_paths = std::vector<Path>{};
    relative_paths.reserve(archive.size());
    for (const auto &entry : archive)
        relative_paths.emplace_back(entry.first);

    auto preview = UnpackPreview{};
    preview.files_to_extract.reserve(relative_paths.size());
    if (settings.overwrite_existing_files)
    {
        preview.files_to_extract = std::move(relative_paths);
        return preview;
    }

    const auto existing_paths    = common::find_matching_paths_icase(root, relative_paths);
    auto existing_relative_paths = std::unordered_set<std::u8string>{};
    existing_relative_paths.reserve(existing_paths.size());
    for (const auto &existing_path : existing_paths)
        existing_relative_paths.emplace(lower_generic_path(existing_path.lexically_relative(root)));

    preview.skipped_files.reserve(existing_relative_paths.size());
    for (const auto &relative_path : relative_paths)
    {
        if (existing_relative_paths.contains(lower_generic_path(relative_path)))
        {
            preview.skipped_files.emplace_back(relative_path);
        }
        else
        {
            preview.files_to_extract.emplace_back(relative_path);
        }
    }

    return preview;
}
} // namespace

auto list_unpack_preview(const UnpackSettings &settings) noexcept
    -> tl::expected<UnpackPreview, common::Error>
{
    auto archive = Archive::read(settings.file_path);
    if (!archive)
        return tl::make_unexpected(archive.error());

    return make_unpack_preview(*archive, settings);
}

auto unpack(UnpackSettings sets) noexcept -> tl::expected<void, Error>
{
    {
        auto arch = Archive::read(sets.file_path);
        if (!arch)
            return tl::make_unexpected(Error(BsaErr::FailedToReadArchive));

        const auto &root = sets.extract_to_dir.value_or(sets.file_path.parent_path());
        const auto preview = make_unpack_preview(*arch, sets);
        auto skipped_paths = std::unordered_set<std::u8string>{};
        skipped_paths.reserve(preview.skipped_files.size());
        for (const auto &path : preview.skipped_files)
            skipped_paths.emplace(lower_generic_path(path));

        common::for_each_mt(std::move(*arch), [root, &sets, &skipped_paths](auto &&elem) {
            const auto path = root / elem.first;
            if (sets.overwrite_existing_files
                || (!skipped_paths.contains(lower_generic_path(elem.first)) && !fs::exists(path)))
            {
                fs::create_directories(path.parent_path());
                const bool res = elem.second.write(path);
                if (!res)
                {
                    // TODO: how to handle error here?
                }
            }
        });
    }
    if (sets.remove_arch && !fs::remove(sets.file_path))
    {
        return tl::make_unexpected(Error(BsaErr::FailedToRemoveArchive));
    }

    return {};
}

} // namespace btu::bsa
