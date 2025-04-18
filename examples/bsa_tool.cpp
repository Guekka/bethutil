/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/bsa/pack.hpp"
#include "btu/bsa/unpack.hpp"

#include <btu/bsa/plugin.hpp>

#include <iostream>

void pack(const btu::Path &dir, const btu::bsa::Settings &sets)
{
    pack(btu::bsa::PackSettings{
             .input_dir     = dir,
             .game_settings = sets,
             .compress      = btu::bsa::Compression::Yes,
         })
        .for_each([&sets, &dir](btu::bsa::Archive &&arch) {
            const auto name = find_archive_name(dir, sets, arch.type());
            if (!name)
            {
                std::cerr << "Failed to find archive name\n";
                return;
            }

            if (!std::move(arch).write(name.value()))
            {
                std::cerr << "Failed to write archive\n";
            }
        });

    make_dummy_plugins(list_archive(dir, sets), sets);
}

void unpack(const btu::Path &dir, const btu::bsa::Settings &sets)
{
    auto archives = list_archive(dir, sets);
    for (const auto &file : archives)
    {
        std::cout << "Unpacking " << file.string() << '\n' << std::flush;
        const auto params = btu::bsa::UnpackSettings{
            .file_path = file,
        };
        if (const auto res = unpack(params); !res)
            std::cerr << "Failed to unpack archive:" << res.error() << '\n' << std::flush;
    }
}

void list(const btu::Path &dir, const btu::bsa::Settings &sets)
{
    auto archives = list_archive(dir, sets);
    for (const auto &file : archives)
    {
        std::cout << "Files of: " << file.string() << '\n' << std::flush;
        auto arch = btu::bsa::Archive::read(file);
        if (!arch)
        {
            std::cerr << "Failed to read archive\n";
            continue;
        }
        for (auto &&elem : std::move(*arch))
        {
            std::cout << elem.first << "  " << elem.second.size().value_or(0) << " bytes - Compressed: "
                      << (elem.second.compressed() == btu::bsa::Compression::Yes ? "Yes" : "No") << '\n';
        }
    }
}

auto process_args(std::vector<std::string_view> args) -> int
{
    auto dir  = btu::fs::current_path();
    auto game = btu::Game::SSE;
    if (args.empty())
    {
        std::cerr << "Bad usage";
        return 1;
    }

    if (args.size() >= 2)
        dir = args[1];

    if (args.size() >= 3)
        game = nlohmann::json(args[2]).get<btu::Game>();

    const auto command = args[0];

    auto func = std::unordered_map<std::string_view,
                                   std::function<void(const btu::Path &, const btu::bsa::Settings &)>>{
        {"pack", pack},
        {"unpack", unpack},
        {"list", list},
    };

    auto sets = btu::bsa::Settings::get(game);
    if (const auto it = func.find(command); it != func.end())
    {
        std::cout << "Processing with parameters:\n"
                  << "Command: " << command << '\n'
                  << "Directory: " << dir.string() << '\n'
                  << "Game: " << nlohmann::json(game) << '\n';
        it->second(dir, sets);
    }
    else
    {
        std::cerr << "Unknown command";
        return 1;
    }

    return 0;
}

auto main(const int argc, char *argv[]) -> int
{
    const auto start = std::chrono::high_resolution_clock::now();
    try
    {
        const auto args = std::vector<std::string_view>(argv + 1, argv + argc);
        process_args(args);
    }
    catch (std::exception const &e)
    {
        std::cerr << "An exception happened: " << e.what();
    }
    const auto end = std::chrono::high_resolution_clock::now();
    std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms elapsed";
}
