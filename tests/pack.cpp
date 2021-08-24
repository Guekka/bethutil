/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/transform.hpp"
#include "btu/bsa/unpack.hpp"

#include <chrono>

// Intended for library devs only

/* Pack
 * 
 * Requiem MT : 6s
 * Requiem ST : 33s
 * unglau MT 755s
*/

/* Unpack
 * Requiem MT C++ : 14959ms
 * Requiem MT Delphi : 6236ms
 * Unglau MT Delphi : 1494s
*/

#ifdef BETHUTIL_BSA_INTERNAL_TEST
//const auto dir = R"(F:\Edgar\Downloads\unglaubliche Reise EXTENDED Version SSE DV 2.0 BSA)";
const auto dir
    = R"(E:\Programmes\Mod_Skyrim_SE\Cathedral Assets Optimizer\TESTS\TES5_TO_SSE\BSACreation\INPUT - Copy)";

template<typename Func>
void run(Func const &f, std::string const &name)
{
    using namespace std::chrono;

    auto start = high_resolution_clock::now();
    try
    {
        f();
    }
    catch (std::exception const &e)
    {
        std::cerr << "e:" << e.what() << std::endl;
    }
    auto end  = high_resolution_clock::now();
    auto time = duration_cast<milliseconds>(end - start).count();
    std::cout << name << " took " << time << "ms" << std::endl;
}

TEST_CASE("Create BSA")
{
    auto sets = Settings::get(Game::SSE);
    //cleanDummyPlugins(dir, sets);
    makeDummyPlugins(dir, sets);
    run([] { create(dir, true, Settings::get(Game::SSE)); }, "create");
    run([] { extractAll(dir, Settings::get(Game::SSE)); }, "extract");
    run(
        [] {
            libbsarch::bsa bsa;
            transform(
                R"(E:\Programmes\Mod_Skyrim_SE\Cathedral Assets Optimizer\TESTS\TES5_TO_SSE\BSACreation\INPUT - Copy\Requiem.bsa)",
                "out.bsa",
                [](auto, auto data) { return libbsarch::to_vector(std::move(data)); },
                Settings::get(Game::SSE));
        },
        "transform");
}
#endif
