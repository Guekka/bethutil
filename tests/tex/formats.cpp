#include "btu/tex/formats.hpp"

#include <catch.hpp>

using btu::tex::BestFormatFor, btu::tex::guess_best_format;
using enum btu::tex::AllowCompressed;

TEST_CASE("guess_texture_type", "[src]")
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

    SECTION("Classification", "[src]")
    {
        // No underscore means diffuse
        CHECK(guess_texture_type(u8"somename.dds") == TextureType::Diffuse);
        // Unknown underscore
        CHECK(guess_texture_type(u8"some_name.dds") == std::nullopt);

        // Case-insensitive
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

constexpr auto k_best_formats = BestFormatFor{.uncompressed               = DXGI_FORMAT_R8G8B8A8_UNORM,
                                              .uncompressed_without_alpha = DXGI_FORMAT_R8G8_UNORM,
                                              .compressed                 = DXGI_FORMAT_BC7_UNORM,
                                              .compressed_without_alpha   = DXGI_FORMAT_BC5_UNORM};

TEST_CASE("guess_best_format compressed with alpha", "[src]")
{
    auto result = guess_best_format(DXGI_FORMAT_BC3_UNORM, k_best_formats, Yes);
    CHECK(result == DXGI_FORMAT_BC7_UNORM);
}

TEST_CASE("guess_best_format compressed without alpha", "[src]")
{
    auto result = guess_best_format(DXGI_FORMAT_BC5_UNORM, k_best_formats, Yes);
    CHECK(result == DXGI_FORMAT_BC5_UNORM);
}

TEST_CASE("guess_best_format uncompressed with alpha", "[src]")
{
    auto result = guess_best_format(DXGI_FORMAT_R8G8B8A8_UNORM, k_best_formats, No);
    CHECK(result == DXGI_FORMAT_R8G8B8A8_UNORM);
}

TEST_CASE("guess_best_format uncompressed without alpha", "[src]")
{
    auto result = guess_best_format(DXGI_FORMAT_R8G8_B8G8_UNORM, k_best_formats, No);
    CHECK(result == DXGI_FORMAT_R8G8_UNORM);
}

TEST_CASE("guess_best_format already compressed with allow compressed no", "[src]")
{
    auto result = guess_best_format(DXGI_FORMAT_BC3_UNORM, k_best_formats, No);
    CHECK(result == DXGI_FORMAT_BC7_UNORM);
}

TEST_CASE("guess_best_format non compressed with allow compressed yes", "[src]")
{
    auto result = guess_best_format(DXGI_FORMAT_R8G8B8A8_UNORM, k_best_formats, Yes);
    CHECK(result == DXGI_FORMAT_BC7_UNORM);
}