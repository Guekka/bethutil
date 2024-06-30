/* Copyright (C) 2020 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <btu/bsa/archive.hpp>
#include <btu/bsa/settings.hpp>
#include <btu/common/error.hpp>
#include <btu/common/functional.hpp>
#include <btu/common/path.hpp>
#include <tl/expected.hpp>

namespace btu::modmanager {

struct ModFile
{
    Path relative_path;
    common::Lazy<tl::expected<std::vector<std::byte>, common::Error>> content;
};

class ModFolderIteratorBase
{
public:
    virtual ~ModFolderIteratorBase() = default;

    enum class ArchiveTooLargeAction : std::uint8_t
    {
        Skip,
        Process,
    };

    enum class ArchiveTooLargeState : std::uint8_t
    {
        BeforeProcessing,
        AfterProcessing,
    };

    [[nodiscard]] virtual auto archive_too_large(
        const Path &archive_path, ArchiveTooLargeState state) noexcept -> ArchiveTooLargeAction = 0;

    virtual void failed_to_read_archive(const Path &archive_path) noexcept {}
};

class ModFolderTransformer : public ModFolderIteratorBase
{
public:
    /// \brief Takes a file content and returns a new content. If the function returns
    /// std::nullopt, the file is not changed.
    /// \note For optimal performance, it is recommended to return std::nullopt for files that do not need to be changed.
    [[nodiscard]] virtual auto transform_file(ModFile file) noexcept -> std::optional<std::vector<std::byte>>
                                                                        = 0;

    virtual void failed_to_write_transformed_file(const Path &relative_path,
                                                  std::span<const std::byte> content) noexcept
    {
    }

    virtual void failed_to_read_transformed_file(const Path &relative_path,
                                                 std::span<const std::byte> content) noexcept
    {
    }

    virtual void failed_to_write_archive(const Path &old_archive_path, const Path &new_archive_path) noexcept
    {
    }
};

class ModFolderIterator : public ModFolderIteratorBase
{
public:
    virtual void process_file(ModFile file) noexcept = 0;
};

class ModFolder
{
public:
    using enum ModFolderIteratorBase::ArchiveTooLargeAction;
    using enum ModFolderIteratorBase::ArchiveTooLargeState;

    explicit ModFolder(Path directory, bsa::Settings bsa_settings);

    /// Get the size of the folder, including files in archives.
    /// Utility function, equivalent to iterate() and counting the files.
    [[nodiscard]] auto size() const noexcept -> size_t;

    /// Transform all files in the folder, including files in archives.
    /// Multithreaded.
    void transform(ModFolderTransformer &transformer) noexcept;

    /// Iterate over all files in the folder, including files in archives.
    /// Multithreaded.
    void iterate(ModFolderIterator &transformer) const noexcept;

    /// Iterate over all files in the folder, including archives.
    /// Multithreaded.
    void iterate(const std::function<void(Path relative_path)> &loose,
                 const std::function<void(const Path &archive_path)> &archive) const noexcept;

    [[nodiscard]] auto name() const noexcept -> std::u8string { return dir_.filename().u8string(); }
    [[nodiscard]] auto path() const noexcept -> const Path & { return dir_; }

private:
    // This function is used to implement transform() and one of the iterate() functions.
    // The reason for it to be private is that while it is `const`, it is not `const` semantically: it modifies the
    // folder
    void transform_impl(ModFolderTransformer &transformer) const noexcept;

    Path dir_;
    bsa::Settings bsa_settings_;
};
} // namespace btu::modmanager
