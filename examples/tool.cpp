/* Copyright (C) 2021 G'k
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/BSACreate.hpp"
#include "btu/bsa/BSAExtract.hpp"

#include <iostream>
#include <string_view>

using namespace btu::bsa;

void processArgs(std::vector<std::string_view> args, path const &dir)
{
    auto const &sets = GameSettings::get(Games::SSE);
    auto const arg   = args.at(0);
    if (arg == "pack")
    {
        auto bsas = splitBSA(dir, true, sets);
        for (auto const &bsa : bsas)
        {
            std::cout << "Packing " << dir.string() << std::endl;
            create(dir, bsa, true, sets);
        }
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
    if (argc != 2)
    {
        std::cerr << "Bad usage";
        return 1;
    }

    try
    {
        auto const args = std::vector<std::string_view>(argv + 1, argv + argc);
        processArgs(args, fs::current_path());
    }
    catch (std::exception const &e)
    {
        std::cerr << "An exception happened: " << e.what();
    }
}
