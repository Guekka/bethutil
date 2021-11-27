#include "btu/tex/formats.hpp"

#include <catch.hpp>

TEST_CASE("guess_texture_type")
{
    using btu::tex::guess_texture_type, btu::tex::TextureType;

    SECTION("String below 6 characters are rejected")
    {
        CHECK(guess_texture_type(u8"") == std::nullopt);
        CHECK(guess_texture_type(u8"a") == std::nullopt);
        CHECK(guess_texture_type(u8"aa") == std::nullopt);
        CHECK(guess_texture_type(u8"aaa") == std::nullopt);
        CHECK(guess_texture_type(u8"aaaa") == std::nullopt);
        CHECK(guess_texture_type(u8"aaaaa") == std::nullopt);
    }

    SECTION("Classification")
    {
        // No underscore means diffuse
        CHECK(guess_texture_type(u8"somename.dds") == TextureType::Diffuse);
        // Unknown underscore
        CHECK(guess_texture_type(u8"some_name.dds") == std::nullopt);

        // Case insensitive
        CHECK(guess_texture_type(u8"some_name_n.dds") == TextureType::Normal);
        CHECK(guess_texture_type(u8"some_name_N.dds") == TextureType::Normal);

        CHECK(guess_texture_type(u8"some_name_s.dds") == TextureType::Specular);
        CHECK(guess_texture_type(u8"some_name_d.dds") == TextureType::Diffuse);
        CHECK(guess_texture_type(u8"some_name_g.dds") == TextureType::Glow);
        CHECK(guess_texture_type(u8"some_name_p.dds") == TextureType::Parallax);
        CHECK(guess_texture_type(u8"some_name_e.dds") == TextureType::Cube);
        CHECK(guess_texture_type(u8"some_name_msn.dds") == TextureType::ModelSpaceNormal);
        CHECK(guess_texture_type(u8"some_name_b.dds") == TextureType::Backlight);
        CHECK(guess_texture_type(u8"some_name_sk.dds") == TextureType::Skin);
        CHECK(guess_texture_type(u8"some_name_em.dds") == TextureType::EnvironmentMask);
        CHECK(guess_texture_type(u8"some_name_m.dds") == TextureType::EnvironmentMask);
    }
}
