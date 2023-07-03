#include "../utils.hpp"

#include <btu/esp/functions.hpp>

TEST_CASE("list_headparts", "[src]")
{
    const auto res = btu::esp::list_headparts("esp/headparts.esp");
    REQUIRE(res);
    REQUIRE(res->size() == 52);

    // no need to check all of them, hopefully this is enough
    CHECK(res->at(00) == u8"actors/character/character assets/eyesfemale.nif");
    CHECK(res->at(10) == u8"actors/character/character assets/hair/femaleredguardhair02.nif");
    CHECK(res->at(20) == u8"actors/character/ranaline/character assets/childmaskrightside.nif");
    CHECK(res->at(30) == u8"actors/character/ranaline/hair/children/female02.nif");
    CHECK(res->at(40) == u8"actors/character/ranaline/hair/children/male03.nif");
    CHECK(res->at(50) == u8"actors/character/ranaline/hair/hairginko04.nif");
}

TEST_CASE("list_landscape_textures", "[src]")
{
    const auto res = btu::esp::list_landscape_textures("esp/landscape.esp");
    REQUIRE(res);
    const auto expected = std::vector<std::u8string>{
        u8"dlc01/landscape/glowingforestdirt01.dds",
        u8"dlc01/landscape/soulcairnbones01.dds",
        u8"dlc01/landscape/soulcairndirt01.dds",
        u8"dlc01/landscape/winterforestleaves02.dds",
    };
    CHECK(*res == expected);
}