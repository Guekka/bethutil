/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <btu/bsa/archive.hpp>
#include <btu/bsa/settings.hpp>
#include <btu/common/path.hpp>

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

    explicit ModFolder(Path directory, btu::bsa::Settings bsa_settings);

    using Transformer = std::function<std::optional<std::vector<std::byte>>(ModFile)>;

    enum class ArchiveTooLargeState
    {
        BeforeProcessing,
        AfterProcessing,
    };

    enum class ArchiveTooLargeAction
    {
        Skip,
        Process,
    };

    using ArchiveTooLargeHandler
        = std::function<ArchiveTooLargeAction(const Path &archive_path, ArchiveTooLargeState state)>;

    /// Get the size of the folder, including files in archives.
    /// Utility function, equivalent to iterate() and counting the files.
    [[nodiscard]] auto size() const -> size_t;

    /// Transform all files in the folder, including files in archives.
    /// Multithreaded.
    /// \arg transformer Function that takes a file content and returns a new content. If the function returns
    /// std::nullopt, the file is not changed.
    /// \note For optimal performance, it is recommended to return std::nullopt for files that do not need to be changed.
    void transform(Transformer &&transformer, ArchiveTooLargeHandler &&archive_too_large_handler);

    /// Iterate over all files in the folder, including files in archives.
    /// Multithreaded.
    void iterate(const std::function<void(ModFile)> &visitor,
                 ArchiveTooLargeHandler &&archive_too_large_handler) const;

    /// Iterate over all files in the folder, including archives.
    /// Multithreaded.
    void iterate(const std::function<void(Path relative_path)> &loose,
                 const std::function<void(const Path &archive_path, bsa::Archive &&archive)> &archive) const;

private:
    // This function is used to implement transform() and one of the iterate() functions.
    // The reason for it to be private is that while it is `const`, it is not `const` semantically: it modifies the
    // folder
    void transform_impl(Transformer &&transformer, ArchiveTooLargeHandler &&archive_too_large_handler) const;

    Path dir_;
    btu::bsa::Settings bsa_settings_;
};
} // namespace btu::modmanager
