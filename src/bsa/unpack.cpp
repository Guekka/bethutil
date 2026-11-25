/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/error_code.hpp"

#include <btu/common/threading.hpp>
#include <tl/expected.hpp>

#include <atomic>

namespace btu::bsa {
auto unpack(UnpackSettings sets) noexcept -> tl::expected<void, Error>
{
    {
        auto arch = Archive::read(sets.file_path);
        if (!arch)
            return tl::make_unexpected(Error(BsaErr::FailedToReadArchive));

        const auto &root = sets.extract_to_dir.value_or(sets.file_path.parent_path());
        std::atomic_bool failed_to_write = false;
        common::for_each_mt(std::move(*arch), [root, &sets, &failed_to_write](auto &&elem) {
            const auto path = root / elem.first;
            std::error_code ec;
            const bool path_exists = fs::exists(path, ec);
            if (ec)
            {
                failed_to_write = true;
                return;
            }

            if (!sets.overwrite_existing_files && path_exists)
                return; // preserve existing loose files

            fs::create_directories(path.parent_path(), ec);
            if (ec || !elem.second.write(path))
                failed_to_write = true;
        });

        if (failed_to_write)
            return tl::make_unexpected(Error(BsaErr::FailedToWriteFile));
    }

    if (sets.remove_arch)
    {
        std::error_code ec;
        if (!fs::remove(sets.file_path, ec) || ec)
            return tl::make_unexpected(Error(BsaErr::FailedToRemoveArchive));
    }

    return {};
}

} // namespace btu::bsa
