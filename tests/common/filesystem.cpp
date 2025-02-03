/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "utils.hpp"

#include <btu/common/filesystem.hpp>
#include <catch.hpp>

#include <fstream>
#include <flux/algorithm/all_any_none.hpp>

namespace fs = btu::fs;

class FsTempPath : public TempPath
{
public:
    FsTempPath()
        : TempPath(fs::temp_directory_path() / "btu_common_filesystem_test")
    {
        fs::create_directories(fs::temp_directory_path() / "btu_common_filesystem_test");
    }
};

class FsTempDir : public FsTempPath
{
public:
    FsTempDir() { fs::create_directories(path()); }
};

class FsTempFile : public FsTempPath
{
public:
    explicit FsTempFile(const std::string &content = "") { create_file(path(), content); }
};

TEST_CASE("read_file", "[src]")
{
    SECTION("invalid path has error")
    {
        const auto data = btu::common::read_file("invalid_path");
        CHECK(!data);
    }
    SECTION("100space.bin")
    {
        const auto file    = FsTempFile();
        const auto content = std::string(100, ' ');
        const auto data    = require_expected(btu::common::read_file(file.path()));
        CHECK(data.size() == fs::file_size(file.path()));
        CHECK(std::equal(data.cbegin(), data.cend(), content.cbegin(), [](auto byte, auto c) {
            return static_cast<std::byte>(c) == byte;
        }));
    }
}

TEST_CASE("hard_link", "[src]")
{
    SECTION("source is a file")
    {
        const auto source      = FsTempFile();
        const auto destination = FsTempPath();
        require_expected(btu::common::hard_link(source.path(), destination.path()));
        CHECK(btu::common::compare_files(source.path(), destination.path()));
    }
    SECTION("source is a directory with files")
    {
        const auto source = FsTempDir();
        create_file(source.path() / "file1", "content1");
        const auto destination = FsTempPath();
        require_expected(btu::common::hard_link(source.path(), destination.path()));
        CHECK(btu::common::compare_directories(source.path(), destination.path()));
    }
    SECTION("source does not exist")
    {
        const auto source      = "invalid_path"sv;
        const auto destination = "destination_path"sv;
        const auto result      = btu::common::hard_link(source, destination);
        CHECK_FALSE(result);
        CHECK_FALSE(fs::exists(destination));
    }
}

TEST_CASE("find_matching_paths_icase", "[src]")
{
    SECTION("directory exists")
    {
        const auto directory                            = FsTempDir();
        std::vector<btu::Path> relative_lowercase_paths = {"file1", "file2"};
        create_file(directory.path() / "file1");
        create_file(directory.path() / "file2");
        const auto result = btu::common::find_matching_paths_icase(directory.path(), relative_lowercase_paths);
        const auto expected = std::vector<btu::Path>{directory.path() / "file1", directory.path() / "file2"};
        CHECK(result == expected);
    }
    SECTION("directory does not exist")
    {
        const auto directory                            = "invalid_path"sv;
        std::vector<btu::Path> relative_lowercase_paths = {"file1", "file2"};
        const auto result = btu::common::find_matching_paths_icase(directory, relative_lowercase_paths);
        CHECK(result.empty());
    }
    SECTION("directory contains files with different case sensitivity")
    {
        const auto directory                            = FsTempDir();
        std::vector<btu::Path> relative_lowercase_paths = {"file1", "FILE2"};
        create_file(directory.path() / "file1");
        create_file(directory.path() / "FILE2");
        const auto result = btu::common::find_matching_paths_icase(directory.path(), relative_lowercase_paths);

        // we cannot compare against expected value since windows and linux have different behavior
        CHECK(result.size() == 2);
        CHECK(flux::all(result, [](const auto &path) { return fs::exists(path); }));
    }
    SECTION("one path among many does not exist")
    {
        const auto directory                            = FsTempDir();
        std::vector<btu::Path> relative_lowercase_paths = {"file1", "file2", "file3"};
        create_file(directory.path() / "file1");
        create_file(directory.path() / "file2");
        const auto result = btu::common::find_matching_paths_icase(directory.path(), relative_lowercase_paths);
        const auto expected = std::vector<btu::Path>{directory.path() / "file1", directory.path() / "file2"};
        CHECK(result == expected);
    }
}

TEST_CASE("compare_directories", "[src]")
{
    SECTION("identical directories")
    {
        const auto dir1 = FsTempDir();
        const auto dir2 = FsTempDir();
        create_file(dir1.path() / "file1", "content1");
        create_file(dir2.path() / "file1", "content1");
        CHECK(btu::common::compare_directories(dir1.path(), dir2.path()));
    }
    SECTION("different directories")
    {
        const auto dir1 = FsTempDir();
        const auto dir2 = FsTempDir();
        create_file(dir1.path() / "file1", "content1");
        create_file(dir2.path() / "file1", "content2");
        CHECK_FALSE(btu::common::compare_directories(dir1.path(), dir2.path()));
    }
    SECTION("directory does not exist")
    {
        const auto dir1 = FsTempDir();
        const auto dir2 = "invalid_path"sv;
        create_file(dir1.path() / "file1", "content1");
        CHECK_FALSE(btu::common::compare_directories(dir1.path(), dir2));
    }
}

TEST_CASE("compare_files", "[src]")
{
    SECTION("identical files")
    {
        const auto file1 = FsTempFile("test content");
        const auto file2 = FsTempFile("test content");
        CHECK(btu::common::compare_files(file1.path(), file2.path()));
    }
    SECTION("different files")
    {
        const auto file1 = FsTempFile("test content 1");
        const auto file2 = FsTempFile("test content 2");
        CHECK_FALSE(btu::common::compare_files(file1.path(), file2.path()));
    }
    SECTION("file does not exist")
    {
        const auto file1 = FsTempFile();
        const auto file2 = "invalid_path"sv;
        CHECK_FALSE(btu::common::compare_files(file1.path(), file2));
    }
}

TEST_CASE("write_file", "[src]")
{
    SECTION("valid path and content")
    {
        const auto file    = FsTempFile();
        const auto content = std::vector(100, static_cast<std::byte>(' '));
        const auto result  = btu::common::write_file(file.path(), content);
        CHECK(result);
        CHECK(fs::file_size(file.path()) == content.size());
    }
}

TEST_CASE("write_file_new", "[src]")
{
    const auto content = std::vector(100, static_cast<std::byte>(' '));

    SECTION("valid path and content")
    {
        const auto file   = FsTempPath();
        const auto result = btu::common::write_file_new(file.path(), content);
        CHECK(result);
        CHECK(fs::file_size(file.path()) == content.size());
    }
    SECTION("file already exists")
    {
        const auto file   = FsTempFile();
        const auto result = btu::common::write_file_new(file.path(), content);
        CHECK_FALSE(result);
    }
}