/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"
#include "btu/bsa/unpack.hpp"

#include <iostream>
#include <string_view>

using namespace btu::bsa;

void process_args(std::vector<std::string_view> args, const Path &dir)
{
    const auto &sets = Settings::get(Game::SSE);
    const auto arg   = args.at(0);
    if (arg == "pack")
    {
        auto bsas = split(dir, sets);
        merge(bsas);
        for (auto bsa : std::move(bsas))
            write(/*compressed=*/true, std::move(bsa), sets, dir);
    }
    else if (arg == "unpack")
    {
        std::vector files(fs::directory_iterator(dir), fs::directory_iterator{});
        erase_if(files, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
        for (const auto &file : files)
        {
            std::cout << "Unpacking " << file.path().string() << std::endl;
            unpack({.file_path = dir});
        }
    }
}

auto main(int argc, char *argv[]) -> int
{
    Path dir = fs::current_path();
    if (argc == 3)
    {
        dir = argv[2];
    }
    else if (argc != 2)
    {
        std::cerr << "Bad usage";
        return 1;
    }

    auto start = std::chrono::high_resolution_clock::now();
    try
    {
        const auto args = std::vector<std::string_view>(argv + 1, argv + argc);
        process_args(args, dir);
    }
    catch (std::exception const &e)
    {
        std::cerr << "An exception happened: " << e.what();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms elapsed";
}
