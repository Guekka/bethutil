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

    const bool success = std::move(*arch).write(path);
    REQUIRE(success);
    REQUIRE(btu::common::compare_directories(dir / "in", dir / "out"));
}

TEST_CASE("set archive version", "[src]")
{
    GIVEN("An archive with a file")
    {
        auto arch = btu::bsa::Archive{btu::bsa::ArchiveVersion::tes3, btu::bsa::ArchiveType::Standard};

        auto file = btu::bsa::File(btu::bsa::ArchiveVersion::tes3,
                                   btu::bsa::ArchiveType::Standard,
                                   std::nullopt);

        REQUIRE(arch.emplace("file", file));
        auto data = std::vector{std::byte{0x00}, std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
        REQUIRE(file.read(data));

        REQUIRE(arch.version() == btu::bsa::ArchiveVersion::tes3);
        REQUIRE(file.version() == btu::bsa::ArchiveVersion::tes3);

        WHEN("The version is set to tes4")
        {
            REQUIRE(arch.set_version(btu::bsa::ArchiveVersion::tes4));

            THEN("The archive and the files inside should have the new version")
            {
                REQUIRE(arch.version() == btu::bsa::ArchiveVersion::tes4);
                REQUIRE(flux::all(flux::from_range(arch), [](auto &&elem) {
                    return elem.second.version() == btu::bsa::ArchiveVersion::tes4;
                }));
            }
        }
    }
}
