/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"
#include "btu/bsa/unpack.hpp"

#include <iostream>
#include <string_view>

using namespace btu::bsa;

void processArgs(std::vector<std::string_view> args, Path const &dir)
{
    auto const &sets = Settings::get(Game::SSE);
    auto const arg   = args.at(0);
    if (arg == "pack")
    {
        create(CreationSettings{
            .dir               = dir,
            .compact_archives  = true,
            .compress_archives = true,
            .settings          = sets,
        });
    }
    else if (arg == "unpack")
    {
        std::vector files(fs::directory_iterator(dir), fs::directory_iterator{});
        erase_if(files, [&sets](auto const &file) { return file.path().extension() != sets.extension; });
        for (auto const &file : files)
        {
            std::cout << "Unpacking " << file.path().string() << std::endl;
            extract(file.path());
        }
    }
}

int main(int argc, char *argv[])
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
        auto const args = std::vector<std::string_view>(argv + 1, argv + argc);
        processArgs(args, dir);
    }
    catch (std::exception const &e)
    {
        std::cerr << "An exception happened: " << e.what();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms elapsed";
}
