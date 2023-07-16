/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/bsa/archive.hpp"
#include "btu/common/path.hpp"

#include <flow.hpp>

#include <variant>

namespace btu::modmanager {
class ModFolder
{
public:
    struct ModFile
    {
        Path relative_path;
        std::vector<std::byte> content;
    };

    explicit ModFolder(Path directory, std::u8string archive_ext);

    /// Get the size of the folder, including files in archives.
    /// Utility function, equivalent to iterate() and counting the files.
    [[nodiscard]] auto size() const -> size_t;

    using Transformer = std::function<std::optional<std::vector<std::byte>>(ModFile)>;
    /// Transform all files in the folder, including files in archives.
    /// Multithreaded.
    /// \arg transformer Function that takes a file content and returns a new content. If the function returns
    /// std::nullopt, the file is not changed.
    /// \note For optimal performance, it is recommended to return std::nullopt for files that do not need to be changed.
    void transform(Transformer &&transformer);

    /// Iterate over all files in the folder, including files in archives.
    /// Multithreaded.
    void iterate(const std::function<void(ModFile)> &visitor) const;

    /// Iterate over all files in the folder, including archives.
    /// Multithreaded.
    void iterate(const std::function<void(Path relative_path)> &loose,
                 const std::function<void(const Path &archive_path, bsa::Archive &&archive)> &archive) const;

private:
    // This function is used to implement transform() and one of the iterate() functions.
    // The reason for it to be private is that while it is `const`, it is not `const` semantically: it modifies the
    // folder
    void transform_impl(Transformer &&transformer) const;

    Path dir_;
    std::u8string archive_ext_;
};
} // namespace btu::modmanager
