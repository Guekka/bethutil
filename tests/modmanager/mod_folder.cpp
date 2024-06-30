/* Copyright (C) 2022 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "btu/modmanager/mod_folder.hpp"

#include "../utils.hpp"
#include "btu/common/filesystem.hpp"

#include <binary_io/memory_stream.hpp>
#include <btu/hkx/anim.hpp>

class Iterator final : public btu::modmanager::ModFolderIterator
{
    Path out_dir_;

public:
    explicit Iterator(Path out_dir)
        : out_dir_(std::move(out_dir))
    {
    }

    [[nodiscard]] auto archive_too_large(const Path & /*archive_path */,
                                         ArchiveTooLargeState /*state*/) noexcept
        -> ArchiveTooLargeAction override
    {
        FAIL("Archive too large, should not happen in tests");
        return ArchiveTooLargeAction::Skip;
    }

    void process_file(btu::modmanager::ModFile f) noexcept override
    {
        const auto out = out_dir_ / f.relative_path;
        btu::fs::create_directories(out.parent_path());
        auto content = require_expected(*f.content);
        require_expected(btu::common::write_file(out, content));
    }
};

TEST_CASE("ModFolder", "[src]")
{
    const Path dir = "modfolder";
    btu::fs::remove_all(dir / "output");

    const auto mf = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
    Iterator iterator{dir / "output"};
    mf.iterate(iterator);
    CHECK(btu::common::compare_directories(dir / "output", dir / "expected"));
}

class Transformer final : public btu::modmanager::ModFolderTransformer
{
public:
    [[nodiscard]] auto archive_too_large(const Path & /*archive_path*/,
                                         ArchiveTooLargeState /*state*/) noexcept
        -> ArchiveTooLargeAction override
    {
        FAIL("Archive too large, should not happen in tests");
        return ArchiveTooLargeAction::Skip;
    }

    [[nodiscard]] auto transform_file(btu::modmanager::ModFile file) noexcept
        -> std::optional<std::vector<std::byte>> override
    {
        // Change one byte in each file
        auto content   = require_expected(*file.content);
        content.back() = std::byte{'0'};
        return content;
    }
};

TEST_CASE("ModFolder transform", "[src]")
{
    const Path dir = "modfolder_transform";
    // operate on copy
    btu::fs::remove_all(dir / "output");
    btu::fs::copy(dir / "input", dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "output", btu::bsa::Settings::get(btu::Game::SSE));

    Transformer transformer;
    mf.transform(transformer);

    CHECK(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder size", "[src]")
{
    const Path dir = "modfolder";
    const auto mf  = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
    CHECK(mf.size() == 4);
}

class IteratorWithArchiveTooLarge final : public btu::modmanager::ModFolderIterator
{
    bool called_ = false;

public:
    [[nodiscard]] auto archive_too_large(const Path & /*archive_path*/,
                                         ArchiveTooLargeState state) noexcept -> ArchiveTooLargeAction override
    {
        CHECK_FALSE(called_);
        CHECK(state == ArchiveTooLargeState::BeforeProcessing);
        called_ = true;
        return ArchiveTooLargeAction::Skip;
    }

    void process_file(btu::modmanager::ModFile /*file*/) noexcept override {}

    [[nodiscard]] auto called() const noexcept -> bool { return called_; }
};

TEST_CASE("Iterate mod folder with archive too big")
{
    const Path dir = "modfolder";
    auto sets      = btu::bsa::Settings::get(btu::Game::FO4);
    sets.max_size  = 1;

    const auto mf = btu::modmanager::ModFolder(dir / "input", sets);
    auto iterator = IteratorWithArchiveTooLarge{};
    mf.iterate(iterator);

    CHECK(iterator.called());
}
