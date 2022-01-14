/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"
#include "btu/bsa/unpack.hpp"

#include <iostream>
#include <string_view>

using namespace btu::bsa;

auto main(int argc, char *argv[]) -> int
{
    std::vector<std::string_view> files(argv + 1, argv + argc);
    for (auto fpath : files)
    {
        Archive a(fpath);
        for (auto f : a.as_flow().to_range())
            std::cout << f.first << '\n';
    }
    std::cout << "\n\n";
}
