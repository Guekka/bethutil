/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/bsa/detail/archive_type.hpp"
#include "btu/bsa/detail/common.hpp"
#include "btu/bsa/settings.hpp"

namespace btu::bsa {
class FilePath
{
    static inline const OsString suffixSeparator = BETHUTIL_BSA_STR(" - ");

public:
    FilePath(Path dir, OsString name, OsString suffix, Path ext, FileTypes type);
    static std::optional<FilePath> make(const Path &path, const Settings &sets, FileTypes type);

    Path full_path() const;
    Path full_name() const;

    Path dir_;
    OsString name_;
    OsString suffix_;
    Path ext_;
    std::optional<uint8_t> counter_;
    FileTypes type_{};

private:
    static std::optional<int> eat_digits(OsString &str);
    static OsString eat_suffix(OsString &str, const Settings &sets);

    explicit FilePath() = default;
};

FilePath find_archive_name(const Path &folderPath, const Settings &sets, ArchiveType type);

void clean_dummy_plugins(const Path &folderPath, const Settings &sets);
void make_dummy_plugins(const Path &folderPath, const Settings &sets);
} // namespace btu::bsa
