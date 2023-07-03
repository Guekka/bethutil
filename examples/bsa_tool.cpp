/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"
#include "btu/bsa/unpack.hpp"

#include <iostream>
#include <string_view>

using namespace btu::bsa;

auto process_args(std::vector<std::string_view> args) -> int
{
    auto dir = btu::fs::current_path();
    if (args.size() == 2)
    {
        dir = args[1];
    }
    else if (args.size() != 1)
    {
        std::cerr << "Bad usage";
        return 1;
    }

    const auto &sets = Settings::get(btu::Game::SSE);
    const auto arg   = args[0];
    const std::vector files(btu::fs::directory_iterator(dir), btu::fs::directory_iterator{});
    if (arg == "pack")
    {
        auto bsas = split(dir, sets);
        merge(bsas);
        auto plugins            = btu::bsa::list_plugins(files.begin(), files.end(), sets);
        const auto default_plug = btu::bsa::FilePath(dir,
                                                     dir.filename().u8string(),
                                                     u8"",
                                                     u8".esp",
                                                     btu::bsa::FileTypes::Plugin);
        if (plugins.empty()) // Used to find BSA name
            plugins.emplace_back(default_plug);

        for (auto bsa : std::move(bsas))
        {
            bsa.set_out_path(btu::bsa::find_archive_name(plugins, sets, bsa.get_type()).full_path());
            write(Compression::Yes, std::move(bsa), dir);
        }
    }
    else if (arg == "unpack")
    {
        auto archives = files;
        erase_if(archives, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
        for (const auto &file : archives)
        {
            std::cout << "Unpacking " << file.path().string() << std::endl;
            unpack({.file_path = dir});
        }
    }
    else if (arg == "list")
    {
        auto archives = files;
        erase_if(archives, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
        for (const auto &file : archives)
        {
            std::cout << "Files of: " << file.path().string() << std::endl;
            auto arch = read_archive(file.path());
            if (!arch)
                continue;
            for (auto &&elem : std::move(*arch))
                std::cout << elem.first + '\n';
        }
    }
    else
    {
        std::cout << "Unknown arg: " << arg;
    }

    return 0;
}

auto main(int argc, char *argv[]) -> int
{
    auto start = std::chrono::high_resolution_clock::now();
    try
    {
        const auto args = std::vector<std::string_view>(argv + 1, argv + argc);
        process_args(args);
    }
    catch (std::exception const &e)
    {
        std::cerr << "An exception happened: " << e.what();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms elapsed";
}
