#include "btu/tex/functions.hpp"

#include "btu/tex/compression_device.hpp"

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <filesystem>
namespace Catch {
template<>
struct StringMaker<btu::tex::Dimension>
{
    static auto convert(const btu::tex::Dimension &value) -> std::string
    {
        auto ts = [](auto &&a) { return StringMaker<decltype(a)>::convert(a); };
        return "{" + ts(value.w) + ", " + ts(value.h) + "}";
    }
};
template<>
struct StringMaker<btu::tex::ScratchImage>
{
    static auto convert(const btu::tex::ScratchImage &) -> std::string { return "scratch_image"; }
};
} // namespace Catch

using btu::tex::Dimension, btu::tex::Texture;

auto load_tex(const std::filesystem::path &path) -> Texture
{
    Texture tex;
    auto res = tex.load_file(path);
    REQUIRE(res.has_value());
    return tex;
}

template<typename Func>
auto test_expected(const std::filesystem::path &root,
                   const std::filesystem::path &filename,
                   Func f,
                   bool approve = false)
{
    auto in                    = load_tex(root / "in" / filename);
    const btu::tex::Result out = f(std::move(in));
    if (!out.has_value())
    {
        UNSCOPED_INFO(out.error().ec.message());
        UNSCOPED_INFO(out.error().loc);
    }
    REQUIRE(out.has_value());

    const auto expected_path = root / "expected" / filename;
    if (!std::filesystem::exists(expected_path) && approve)
    {
        const auto res = out.value().save_file(expected_path);
        CHECK(res.has_value());
        FAIL("Expected file not found:" + expected_path.string());
    }
    else
    {
        const auto expected = load_tex(expected_path);
        CHECK(out->get() == expected.get());
    }
}

template<typename Func>
auto test_expected_dir(const std::filesystem::path &root, const Func &f) -> void
{
    for (const auto &file : std::filesystem::directory_iterator(root / "in"))
        test_expected(root, file.path().filename(), f);
}

TEST_CASE("decompress")
{
    test_expected_dir(u8"decompress", btu::tex::decompress);
}

TEST_CASE("make_opaque_alpha")
{
    test_expected_dir(u8"make_opaque_alpha", btu::tex::make_opaque_alpha);
}
TEST_CASE("convert")
{
    test_expected_dir(u8"convert", [](auto &&tex) {
        auto c = btu::tex::CompressionDevice::make(0);
        return btu::tex::convert(std::move(tex), DXGI_FORMAT_BC7_UNORM, *c);
    });
}
TEST_CASE("generate_mipmaps")
{
    test_expected_dir(u8"generate_mipmaps", btu::tex::generate_mipmaps);
}
TEST_CASE("resize")
{
    test_expected_dir(u8"resize", [](auto &&tex) {
        const auto args = btu::tex::util::ResizeRatio{3, {200, 200}};
        const auto dim  = btu::tex::util::compute_resize_dimension(tex.get_dimension(), args);
        return btu::tex::resize(std::move(tex), dim);
    });
}
TEST_CASE("optimal_mip_count")
{
    using btu::tex::optimal_mip_count;
    STATIC_REQUIRE(optimal_mip_count({1024, 1024}) == 11);
    STATIC_REQUIRE(optimal_mip_count({2048, 1024}) == 12);
    STATIC_REQUIRE(optimal_mip_count({1, 1}) == 1);
}

TEST_CASE("is_pow2")
{
    constexpr size_t limit = 1'000'000;

    using btu::tex::util::is_pow2;

    SECTION("Power of 2 are detected")
    {
        for (size_t i = 1; i < limit; i *= 2)
            CHECK(is_pow2(i));
    }

    SECTION("Non power of 2 are not detected")
    {
        size_t counter{};
        for (size_t i = 0; i < limit; ++i)
            counter += is_pow2(i) ? 1 : 0;

        CHECK(counter == 20);
    }
}

TEST_CASE("upper_pow2")
{
    using btu::tex::util::upper_pow2;

    STATIC_REQUIRE(upper_pow2(0) == 1);
    STATIC_REQUIRE(upper_pow2(1) == 1);
    STATIC_REQUIRE(upper_pow2(3) == 4);
    STATIC_REQUIRE(upper_pow2(458) == 512);
    STATIC_REQUIRE(upper_pow2(8524) == 16384);
}

TEST_CASE("nearest_pow2")
{
    using btu::tex::util::nearest_pow2;

    STATIC_REQUIRE(nearest_pow2(0) == 0);
    STATIC_REQUIRE(nearest_pow2(178) == 128);
    STATIC_REQUIRE(nearest_pow2(150) == 128);
    STATIC_REQUIRE(nearest_pow2(254) == 256);
    // Midpoint returns upper
    STATIC_REQUIRE(nearest_pow2((2 + 4) / 2) == 4);
    STATIC_REQUIRE(nearest_pow2((512 + 1024) / 2) == 1024);
}

TEST_CASE("scale_fit")
{
    using btu::tex::util::scale_fit;
    size_t x = 10, y = 20, tx = 2;
    scale_fit(x, tx, y);
    CHECK(x == 2);
    CHECK(y == 4);
}

TEST_CASE("sanitize_dimensions")
{
    using btu::tex::util::sanitize_dimensions;

    // Already a power of 2
    STATIC_REQUIRE(sanitize_dimensions({512, 1024}) == Dimension{512, 1024});

    // Within treshold of a power of 2
    STATIC_REQUIRE(sanitize_dimensions({504, 516}) == Dimension{512, 512});

    // Within treshold of perfect ratio
    STATIC_REQUIRE(sanitize_dimensions({98, 101}) == Dimension{128, 128});

    // Cannot be simplified
    STATIC_REQUIRE(sanitize_dimensions({144, 200}) == Dimension{144, 200});

    // Or can be, with high threshold
    STATIC_REQUIRE(sanitize_dimensions({144, 200}, 50) == Dimension{128, 128});

    // Cannot be simplified
    STATIC_REQUIRE(sanitize_dimensions({1066, 576}) == Dimension{1066, 576});
}

TEST_CASE("compute_resize_dimension")
{
    using btu::tex::util::compute_resize_dimension, btu::tex::util::ResizeRatio;

    // In theory, we could use STATIC_REQUIRE instead of CHECK
    // But an ICE in MSVC prevents that

    SECTION("Ratio")
    {
        constexpr auto args = ResizeRatio{2, {512, 512}};
        CHECK(compute_resize_dimension({1024, 1024}, args) == Dimension{512, 512});
        CHECK(compute_resize_dimension({512, 512}, args) == Dimension{512, 512});
        CHECK(compute_resize_dimension({700, 700}, args) == Dimension{512, 512});
        CHECK(compute_resize_dimension({700, 900}, args) == Dimension{512, 658});

        // More esoteric
        CHECK(compute_resize_dimension({1066, 576}, args) == Dimension{947, 512});

        constexpr auto args2 = ResizeRatio{5, {25, 25}};
        CHECK(compute_resize_dimension({1024, 1024}, args2) == Dimension{256, 256});
        CHECK(compute_resize_dimension({512, 512}, args2) == Dimension{128, 128});
        CHECK(compute_resize_dimension({100, 100}, args2) == Dimension{32, 32});
        CHECK(compute_resize_dimension({64, 66}, args2) == Dimension{32, 33});

        constexpr auto args3 = ResizeRatio{1, {0, 0}};
        CHECK(compute_resize_dimension({1024, 1024}, args3) == Dimension{1024, 1024});
        // (Almost) square textures are rounded to a power of two
        CHECK(compute_resize_dimension({100, 100}, args3) == Dimension{128, 128});
        CHECK(compute_resize_dimension({64, 65}, args3) == Dimension{64, 64});

        const auto args4 = btu::tex::util::ResizeRatio{3, {200, 200}};
        CHECK(compute_resize_dimension({256, 256}, args4) == Dimension{256, 256});
    }
    SECTION("Absolute")
    {
        constexpr auto args = Dimension{512, 512};

        CHECK(compute_resize_dimension({1024, 1024}, args) == Dimension{512, 512});
        CHECK(compute_resize_dimension({512, 512}, args) == Dimension{512, 512});

        CHECK(compute_resize_dimension({700, 700}, args) == Dimension{512, 512});
        CHECK(compute_resize_dimension({1400, 1400}, args) == Dimension{512, 512});

        CHECK(compute_resize_dimension({1600, 1400}, args) == Dimension{585, 512});
    }
}
