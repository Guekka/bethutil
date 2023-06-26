/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include "btu/common/path.hpp"

#include <fstream>
#include <span>
#include <vector>

namespace btu::common {
[[nodiscard]] inline auto read_file(const Path &a_path) -> std::vector<std::byte>
{
    std::vector<std::byte> data;
    data.resize(fs::file_size(a_path));

    std::ifstream in{a_path, std::ios_base::in | std::ios_base::binary};
    in.exceptions(std::ios_base::failbit);
    in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size())); // NOLINT

    return data;
}

inline void write_file(const Path &a_path, std::span<const std::byte> data)
{
    std::ofstream out{a_path, std::ios_base::binary};
    out.exceptions(std::ios_base::failbit);
    out.write(reinterpret_cast<const char *>(data.data()), // NOLINT
              static_cast<std::streamsize>(data.size()));
}

[[nodiscard]] inline auto compare_files(const Path &filename1, const Path &filename2) -> bool
{
    std::ifstream file1(filename1, std::ifstream::ate | std::ifstream::binary); //open file at the end
    std::ifstream file2(filename2, std::ifstream::ate | std::ifstream::binary); //open file at the end

    if (file1.tellg() != file2.tellg())
    {
        return false; //different file size
    }

    file1.seekg(0); //rewind
    file2.seekg(0); //rewind

    std::istreambuf_iterator<char> begin1(file1);
    std::istreambuf_iterator<char> begin2(file2);

    return std::equal(begin1,
                      std::istreambuf_iterator<char>(),
                      begin2); //Second argument is end-of-range iterator
}

[[nodiscard]] inline auto compare_directories(const Path &dir1, const Path &dir2) -> bool
{
    // sort before comparing, as the directory iteration order is not guaranteed
    auto files1 = std::vector(fs::recursive_directory_iterator(dir1), fs::recursive_directory_iterator{});
    auto files2 = std::vector(fs::recursive_directory_iterator(dir2), fs::recursive_directory_iterator{});

    if (files1.size() != files2.size())
        return false;

    std::sort(files1.begin(), files1.end());
    std::sort(files2.begin(), files2.end());

    auto beg1 = files1.begin();
    auto beg2 = files2.begin();

    while (beg1 != files1.end()) // no need to check beg2, as we already checked the size
    {
        auto path1 = beg1->path();
        auto path2 = beg2->path();

        if (path1.lexically_relative(dir1) != path2.lexically_relative(dir2))
            return false;

        if (beg1->is_directory() != beg2->is_directory())
            return false;

        // Skip directories, we only care about files
        if (beg1->is_directory())
        {
            ++beg1;
            ++beg2;
            continue;
        }

        if (!compare_files(path1, path2))
            return false;

        ++beg1;
        ++beg2;
    }
    return beg1 == files1.end() && beg2 == files2.end();
}

inline void hard_link(const Path &from, const Path &to)
{
    // simple case
    if (!fs::is_directory(from))
    {
        auto ec = std::error_code{};
        fs::create_hard_link(from, to, ec);

        if (ec)
        {
            // we have to make a copy, unfortunately
            fs::copy(from, to);
        }
        return;
    }

    // we cannot hard link directories on Windows, so we create a directory and hardlink files inside
    fs::create_directories(to);
    for (const auto &e : fs::recursive_directory_iterator(from))
        hard_link(e.path(), to / fs::relative(e.path(), from));
}

} // namespace btu::common
