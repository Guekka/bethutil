#include "./utils.hpp"

#include <btu/common/filesystem.hpp>

TEST_CASE("Load and save to same location works", "[src]")
{
    const btu::Path dir = "bsa_load_save";
    btu::fs::remove_all(dir / "out");
    btu::fs::copy(dir / "in", dir / "out");

    const auto path = dir / "out" / "arch.bsa";

    auto arch = btu::bsa::read_archive(path);
    REQUIRE(arch.has_value());

    btu::bsa::write_archive(*BTU_MOV(arch), path);

    REQUIRE(btu::common::compare_directories(dir / "in", dir / "out"));
}

TEST_CASE("set archive version", "[src]")
{
    auto arch = btu::bsa::Archive{};
    auto file = btu::bsa::File{btu::bsa::ArchiveVersion::tes3};
    auto data = std::vector<std::byte>{1};
    file.read(data);

    arch.emplace("test.txt", BTU_MOV(file));

    REQUIRE(btu::bsa::archive_version(arch) == btu::bsa::ArchiveVersion::tes3);

    btu::bsa::set_archive_version(arch, btu::bsa::ArchiveVersion::tes4);

    REQUIRE(btu::bsa::archive_version(arch) == btu::bsa::ArchiveVersion::tes4);
}
