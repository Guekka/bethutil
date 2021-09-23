/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"

#include "btu/bsa/transform.hpp"
#include "btu/bsa/unpack.hpp"

#include <catch2/catch.hpp>

#include <chrono>
#include <iostream>

using namespace btu::bsa;

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
#define BETHUTIL_BSA_INTERNAL_TEST
#ifdef BETHUTIL_BSA_INTERNAL_TEST
const auto dir
    // = R"(E:\Programmes\Mod_Skyrim_SE\Cathedral Assets Optimizer\TESTS\TES5_TO_SSE\BSACreation\INPUT - Copy)";
    //= R"(F:\Edgar\Downloads\mods\unglaubliche Reise EXTENDED Version SSE DV 2.0 BSA)";
    // = R"(C:\Data\BSA)";
    = R"(C:\Skyrim\Chanterelle\Data - Copy)";

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

std::string time()
{
    auto now       = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "[" << std::put_time(std::localtime(&in_time_t), "%H %M %S") << "] ";
    return ss.str();
}

TEST_CASE("Create BSA")
{
    auto sets = Settings::get(Game::SSE);
    run([&] { split(dir, sets); }, "Split");
    //cleanDummyPlugins(dir, sets);
    //makeDummyPlugins(dir, sets);
    run(
        [&sets] {
            std::cout << time() << "start split";
            auto bsas = split(dir, sets);
            merge(bsas);
            for (auto bsa : std::move(bsas))
            {
                std::cout << time() << bsa.find_name(dir, sets) << std::endl;
                const auto errs = write(true, std::move(bsa), sets, dir);
                for (auto &&err : errs)
                {
                    std::wcout << err.first;
                    std::cout << " failed: " << err.second << std::endl;
                }
            }
        },
        "create");

    run(
        [&sets] {
            std::vector files(fs::directory_iterator(dir), fs::directory_iterator{});
            erase_if(files, [&sets](const auto &file) { return file.path().extension() != sets.extension; });
            const auto out = dir / Path("tmp");
            std::for_each(files.begin(), files.end(), [&out](const auto &file) {
                std::cout << time() << file.path().generic_string() << std::endl;
                btu::bsa::unpack(UnpackSettings{.file_path                = file.path(),
                                                .overwrite_existing_files = true,
                                                .root_opt                 = &out});
            });
        },
        "extract");

    /*run(
        [] {
            libbsarch::bsa bsa;
            transform(
                R"(E:\Programmes\Mod_Skyrim_SE\Cathedral Assets Optimizer\TESTS\TES5_TO_SSE\BSACreation\INPUT - Copy\Requiem.bsa)",
                "out.bsa",
                [](auto, auto data) { return libbsarch::to_vector(std::move(data)); },
                Settings::get(Game::SSE));
        },
        "transform");*/
}
#endif
