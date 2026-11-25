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

    void process_file(const btu::modmanager::ModFile f) noexcept override
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

    auto mf = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
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

    [[nodiscard]] auto transform_file(const btu::modmanager::ModFile file) noexcept
        -> std::optional<std::vector<std::byte>> override
    {
        // Change one byte in each file
        auto content   = require_expected(*file.content);
        content.back() = std::byte{'0'};
        return content;
    }
};

TEST_CASE("ModFolder transform with one worker")
{
    const Path dir = "modfolder_transform";
    TempPath temp{dir};
    btu::fs::copy(dir / "input", temp.path());

    auto mf = btu::modmanager::ModFolder(temp.path(), btu::bsa::Settings::get(btu::Game::SSE), false, 1);

    Transformer transformer;
    mf.transform(transformer);

    CHECK(btu::common::compare_directories(temp.path(), dir / "expected"));
}

class FailingTaskRecorder final : public btu::modmanager::ModFolderTransformer
{
    Path archive_path_;
    mutable bool archive_removed_ = false;
    bool failure_reported_        = false;

public:
    explicit FailingTaskRecorder(Path archive_path)
        : archive_path_(std::move(archive_path))
    {
    }

    [[nodiscard]] auto archive_too_large(const Path & /*archive_path*/,
                                         ArchiveTooLargeState /*state*/) noexcept
        -> ArchiveTooLargeAction override
    {
        return ArchiveTooLargeAction::Process;
    }

    [[nodiscard]] auto transform_file(const btu::modmanager::ModFile /*file*/) noexcept
        -> std::optional<std::vector<std::byte>> override
    {
        return std::nullopt;
    }

    [[nodiscard]] auto stop_requested() const noexcept -> bool override
    {
        if (!archive_removed_)
        {
            std::error_code ec;
            archive_removed_ = btu::fs::remove(archive_path_, ec) && !ec;
        }
        return false;
    }

    void failed_to_process_file(const Path &path, const std::exception &error) noexcept override
    {
        failure_reported_ = path == archive_path_ && error.what()[0] != '\0';
    }

    [[nodiscard]] auto archive_removed() const noexcept -> bool { return archive_removed_; }
    [[nodiscard]] auto failure_reported() const noexcept -> bool { return failure_reported_; }
};

TEST_CASE("ModFolder reports worker exceptions")
{
    const Path dir = "modfolder";
    TempPath temp{dir};
    btu::fs::create_directories(temp.path());

    const auto archive_path = temp.path() / "disappears.ba2";
    create_file(archive_path);

    auto mf          = btu::modmanager::ModFolder(temp.path(), btu::bsa::Settings::get(btu::Game::FO4));
    auto transformer = FailingTaskRecorder{archive_path};
    mf.transform(transformer);

    CHECK(transformer.archive_removed());
    CHECK(transformer.failure_reported());
}

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

TEST_CASE("ModFolder ignore existing", "[src]")
{
    const Path dir = "modfolder_ignore_existing";
    // operate on copy
    btu::fs::remove_all(dir / "output");
    btu::fs::copy(dir / "input", dir / "output");

    auto mf = btu::modmanager::ModFolder(dir / "output", btu::bsa::Settings::get(btu::Game::SSE), true);

    Transformer transformer;
    mf.transform(transformer);

    CHECK(btu::common::compare_directories(dir / "output", dir / "expected"));
}

TEST_CASE("ModFolder size", "[src]")
{
    const Path dir = "modfolder";
    auto mf        = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
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

    auto mf       = btu::modmanager::ModFolder(dir / "input", sets);
    auto iterator = IteratorWithArchiveTooLarge{};
    mf.iterate(iterator);

    CHECK(iterator.called());
}

class StoppingIterator final : public btu::modmanager::ModFolderIterator
{
public:
    [[nodiscard]] auto archive_too_large(const Path & /*archive_path*/,
                                         ArchiveTooLargeState /*state*/) noexcept
        -> ArchiveTooLargeAction override
    {
        return ArchiveTooLargeAction::Process;
    }

    void process_file(btu::modmanager::ModFile /*file*/) noexcept override { processed_file_ = true; }
    [[nodiscard]] auto stop_requested() const noexcept -> bool override { return true; }

    [[nodiscard]] auto processed_file() const noexcept -> bool { return processed_file_; }

private:
    bool processed_file_ = false;
};

TEST_CASE("Iteration can be stopped early")
{
    const Path dir = "modfolder";
    auto mf        = btu::modmanager::ModFolder(dir / "input", btu::bsa::Settings::get(btu::Game::FO4));
    auto iterator  = StoppingIterator{};
    mf.iterate(iterator);

    CHECK_FALSE(iterator.processed_file());
}

// Oops, this happened
TEST_CASE("ModFolder transform does not remove the archive it just written because of a case difference")
{
    const Path dir = "modfolder_transform";
    const Path out = dir / "output";
    // operate on copy
    btu::fs::remove_all(out);
    btu::fs::copy(dir / "input", out);

    // expected_fo4.bsa and .BSA point to the same file but are different path
    btu::fs::rename(out / "expected_fo4.ba2", out / "expected_fo4.BA2");
    // and with these settings, the archive will be written as .ba2
    auto sets = btu::bsa::Settings::get(btu::Game::FO4);

    auto mf = btu::modmanager::ModFolder(dir / "output", sets);

    Transformer transformer;
    mf.transform(transformer);

    CHECK(exists(out / "expected_fo4.ba2"));
}