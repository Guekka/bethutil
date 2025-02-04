/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/bsa/error_code.hpp"

#include <btu/common/threading.hpp>
#include <tl/expected.hpp>

namespace btu::bsa {
auto unpack(UnpackSettings sets) -> tl::expected<void, Error>
{
    {
        auto arch = Archive::read(sets.file_path);
        if (!arch)
            return tl::make_unexpected(Error(BsaErr::FailedToReadArchive));

        const auto &root = sets.extract_to_dir.value_or(sets.file_path.parent_path());
        common::for_each_mt(std::move(*arch), [root, &sets](auto &&elem) {
            const auto path = root / elem.first;
            if (sets.overwrite_existing_files || !fs::exists(path)) // preserve existing loose files
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
