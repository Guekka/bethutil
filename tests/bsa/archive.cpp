#include "./utils.hpp"

#include <btu/common/filesystem.hpp>

TEST_CASE("Load and save to same location works", "[src]")
{
    const Path dir = "bsa_load_save";
    btu::fs::remove_all(dir / "out");
    btu::fs::copy(dir / "in", dir / "out");

    const auto path = dir / "out" / "arch.bsa";

    auto arch = btu::bsa::Archive::read(path);
    REQUIRE(arch.has_value());

    std::move(*arch).write(path);

    REQUIRE(btu::common::compare_directories(dir / "in", dir / "out"));
}

TEST_CASE("set archive version", "[src]")
{
    auto arch = btu::bsa::Archive{btu::bsa::ArchiveVersion::tes3, btu::bsa::ArchiveType::Standard};

    auto &file = arch.get("file");
    auto data  = std::vector{std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    file.read(data);

    REQUIRE(arch.version() == btu::bsa::ArchiveVersion::tes3);
    REQUIRE(file.version() == btu::bsa::ArchiveVersion::tes3);

    arch.set_version(btu::bsa::ArchiveVersion::tes4);

    REQUIRE(arch.version() == btu::bsa::ArchiveVersion::tes4);
    REQUIRE(file.version() == btu::bsa::ArchiveVersion::tes4);
}
