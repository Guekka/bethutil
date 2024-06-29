/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/unpack.hpp"

#include "btu/bsa/archive.hpp"
#include "btu/common/functional.hpp"

namespace btu::bsa {
auto unpack(UnpackSettings sets) -> UnpackResult
{
    {
        auto arch = Archive::read(sets.file_path);
        if (!arch)
            return UnpackResult::UnreadableArchive;

        const auto &root = sets.root_opt != nullptr ? *sets.root_opt : sets.file_path.parent_path();
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
        return UnpackResult::FailedToDeleteArchive;
    }

    return UnpackResult::Success;
}

} // namespace btu::bsa
