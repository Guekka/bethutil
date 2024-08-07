#include "./utils.hpp"

#include <btu/common/filesystem.hpp>
#include <btu/tex/functions.hpp>

#include <filesystem>

using btu::tex::Dimension, btu::tex::Texture;

TEST_CASE("Tex Memory IO", "[src]")
{
    const auto dir  = Path{"tex_memory_io"};
    const auto file = dir / "in" / u8"tex.dds";

    // load
    auto data    = require_expected(btu::common::read_file(file));
    auto mem_tex = btu::tex::load(file, data);
    auto fs_tex  = btu::tex::load(file);

    REQUIRE(*mem_tex == *fs_tex);

    // save
    auto mem_data = save(mem_tex.value());
    REQUIRE(mem_data.has_value());

    auto out = dir / "out" / u8"tex.dds";
    btu::fs::create_directories(out.parent_path());

    REQUIRE(btu::tex::save(mem_tex.value(), out));

    auto fs_data = btu::common::read_file(out);

    REQUIRE(*mem_data == fs_data);
}

TEST_CASE("decompress", "[src]")
{
    test_expected_dir(u8"decompress", btu::tex::decompress);
}

TEST_CASE("make_transparent_alpha", "[src]")
{
    test_expected_dir(u8"make_transparent_alpha", btu::tex::make_transparent_alpha);
}

TEST_CASE("convert", "[src]")
{
    SECTION("bc7")
    {
        SECTION("With compression device")
        {
            test_expected_dir(u8"convert", [](auto &&tex) {
                return btu::tex::convert(std::forward<decltype(tex)>(tex),
                                         DXGI_FORMAT_BC7_UNORM,
                                         compression_dev);
            });
        }
        SECTION("Without compression device")
        {
            // TODO
            test_expected_dir(u8"convert2", [](auto &&tex) {
                return btu::tex::convert(std::forward<decltype(tex)>(tex),
                                         DXGI_FORMAT_BC7_UNORM,
                                         compression_dev);
            });
        }
    }
    SECTION("bc1")
    {
        test_expected(u8"convert_bc1", u8"01.dds", [](auto &&tex) {
            return btu::tex::convert(std::forward<decltype(tex)>(tex), DXGI_FORMAT_BC1_UNORM, compression_dev);
        });
    }
}

TEST_CASE("generate_mipmaps", "[src]")
{
    test_expected_dir(u8"generate_mipmaps", btu::tex::generate_mipmaps);
}
TEST_CASE("resize", "[src]")
{
    test_expected_dir(u8"resize", [](auto &&tex) {
        constexpr auto args = btu::tex::util::ResizeRatio{3, {200, 200}};
        const auto dim      = btu::tex::util::compute_resize_dimension(tex.get_dimension(), args);
        return btu::tex::resize(std::forward<decltype(tex)>(tex), dim);
    });
}
TEST_CASE("optimal_mip_count", "[src]")
{
    using btu::tex::optimal_mip_count;
    STATIC_REQUIRE(optimal_mip_count({1024, 1024}) == 11);
    STATIC_REQUIRE(optimal_mip_count({2048, 1024}) == 12);
    STATIC_REQUIRE(optimal_mip_count({1, 1}) == 1);
}

TEST_CASE("is_pow2", "[src]")
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

TEST_CASE("upper_pow2", "[src]")
{
    using btu::tex::util::upper_pow2;

    STATIC_REQUIRE(upper_pow2(0) == 1);
    STATIC_REQUIRE(upper_pow2(1) == 1);
    STATIC_REQUIRE(upper_pow2(3) == 4);
    STATIC_REQUIRE(upper_pow2(458) == 512);
    STATIC_REQUIRE(upper_pow2(8524) == 16384);
}

TEST_CASE("nearest_pow2", "[src]")
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

TEST_CASE("scale_fit", "[src]")
{
    using btu::tex::util::scale_fit;
    size_t x            = 10;
    size_t y            = 20;
    constexpr size_t tx = 2;
    scale_fit(x, tx, y);
    CHECK(x == 2);
    CHECK(y == 4);
}

TEST_CASE("canonize_path", "[src]")
{
    using btu::tex::canonize_path;
    CHECK(canonize_path("a/b/textures/x/y.dds") == u8"x/y.dds");
    CHECK(canonize_path("a\\b\\Textures\\x\\y.dds") == u8"x/y.dds");
    CHECK(canonize_path("x/y.dds") == u8"x/y.dds");
}

TEST_CASE("sanitize_dimensions", "[src]")
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

TEST_CASE("compute_resize_dimension", "[src]")
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

        constexpr auto args4 = ResizeRatio{3, {200, 200}};
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
