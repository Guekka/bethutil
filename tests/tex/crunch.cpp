#include "./utils.hpp"

#include <btu/common/filesystem.hpp>
#include <btu/tex/crunch_functions.hpp>
#include <btu/tex/crunch_texture.hpp>

#include <filesystem>

inline auto load_tex_crunch(const Path &path) -> btu::tex::CrunchTexture
{
    auto res = btu::tex::load_crunch(path);
    INFO(path);
    REQUIRE(res.has_value());
    return std::move(res).value();
}

template<typename Func>
auto test_expected_crunch(const Path &root, const Path &filename, Func f)
{
    auto in                          = load_tex_crunch(root / "in" / filename);
    const btu::tex::ResultCrunch out = f(std::move(in));

    INFO("Processing: " << filename);
    if (!out)
    {
        FAIL_CHECK("Error: " << out.error());
    }

    const auto expected_path = root / "expected" / filename;
    const auto expected      = load_tex_crunch(expected_path);
    INFO("Expected path: " << expected_path);

    CHECK(out.value().get() == expected.get());
}

template<typename Func>
void test_expected_dir_crunch(const Path &root, const Func &f)
{
    const auto in_dir = root / "in";
    for (const auto &file : btu::fs::recursive_directory_iterator(in_dir))
        if (file.is_regular_file())
            test_expected_crunch(root, file.path().lexically_relative(in_dir), f);
}

using btu::tex::Dimension, btu::tex::CrunchTexture;

TEST_CASE("Crunch Tex Memory IO", "[src]")
{
    const auto dir  = Path{"crunch"};
    const auto file = dir / "in" / u8"rock.dds";

    // load
    auto data    = require_expected(btu::common::read_file(file));
    auto mem_tex = require_expected(btu::tex::load_crunch(file, data));
    auto fs_tex  = require_expected(btu::tex::load_crunch(file));

    // test that loaded tex has the correct dimension
    REQUIRE(fs_tex.get_dimension() == Dimension{256, 256});

    REQUIRE(mem_tex == fs_tex);

    // save
    auto mem_data = btu::tex::save(mem_tex);
    REQUIRE(mem_data.has_value());

    auto out = dir / "out" / u8"tex.dds";
    btu::fs::create_directories(out.parent_path());

    REQUIRE(btu::tex::save(mem_tex, out));

    auto fs_data = btu::common::read_file(out);

    REQUIRE(*mem_data == fs_data);
}

TEST_CASE("Crunch Tex has_opaque_alpha", "[src]")
{
    const auto dir                = Path("crunch");
    const auto opaque_file        = dir / "in" / u8"opaque_alpha.dds";
    const auto opaque_file_bc     = dir / "in" / u8"opaque_alpha_compressed.dds";
    const auto not_opaque_file    = dir / "in" / u8"not_opaque_alpha.dds";
    const auto not_opaque_file_bc = dir / "in" / u8"not_opaque_alpha_compressed.dds";

    // Load.
    auto opaque        = require_expected(btu::tex::load_crunch(opaque_file));
    auto opaque_bc     = require_expected(btu::tex::load_crunch(opaque_file_bc));
    auto not_opaque    = require_expected(btu::tex::load_crunch(not_opaque_file));
    auto not_opaque_bc = require_expected(btu::tex::load_crunch(not_opaque_file_bc));

    // Test alpha.
    CHECK(opaque.has_opaque_alpha());
    CHECK(opaque_bc.has_opaque_alpha());
    CHECK_FALSE(not_opaque.has_opaque_alpha());
    CHECK_FALSE(not_opaque_bc.has_opaque_alpha());
}

TEST_CASE("Crunch resize", "[src]")
{
    test_expected_dir_crunch(u8"crunch_resize", [](auto &&tex) {
        constexpr auto args = btu::tex::util::ResizeRatio{3, {200, 200}};
        const auto dim      = btu::tex::util::compute_resize_dimension(tex.get_dimension(), args);
        return btu::tex::resize(std::forward<decltype(tex)>(tex), dim);
    });
}

TEST_CASE("Crunch mipmaps", "[src]")
{
    test_expected_dir_crunch(u8"crunch_mipmaps", btu::tex::generate_mipmaps);
}

TEST_CASE("Crunch convert", "[src]")
{
    SECTION("dxt1")
    {
        test_expected_dir_crunch(u8"crunch_compress_dxt1", [](auto &&tex) {
            return btu::tex::convert(std::forward<decltype(tex)>(tex), DXGI_FORMAT_BC1_UNORM);
        });
    }
    SECTION("dxt5")
    {
        test_expected_dir_crunch(u8"crunch_compress_dxt5", [](auto &&tex) {
            return btu::tex::convert(std::forward<decltype(tex)>(tex), DXGI_FORMAT_BC3_UNORM);
        });
    }
}
