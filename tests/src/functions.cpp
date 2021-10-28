#include "btu/tex/functions.hpp"

#include <DirectXTex.h>

#define CATCH_CONFIG_MAIN

#include <catch2/catch.hpp>

#include <filesystem>

auto load_tex(const std::filesystem::path &path)
{
    DirectX::ScratchImage in;
    DirectX::TexMetadata info{};
    const auto res = DirectX::LoadFromDDSFile(path.wstring().c_str(), DirectX::DDS_FLAGS_NONE, &info, in);
    REQUIRE(SUCCEEDED(res));
    return in;
}

template<typename Func>
auto test_expected(const std::filesystem::path &root, const Func &f) -> void
{
    auto it = std::filesystem::directory_iterator(root / "in");
    for (const auto &file : it)
    {
        const auto in  = load_tex(file.path());
        const auto out = f(in);
        REQUIRE(out.has_value());
        const auto expected = load_tex(root / "expected" / file.path().filename());
        CHECK(*out == expected);
    }
}

// TODO add test files

TEST_CASE("decompress")
{
    test_expected(u8"decompress", btu::tex::decompress);
}
TEST_CASE("make_opaque_alpha")
{
    test_expected(u8"decompress", btu::tex::decompress);
}
TEST_CASE("convert")
{
    test_expected(u8"decompress", btu::tex::decompress);
}
TEST_CASE("generate_mipmaps")
{
    test_expected(u8"decompress", btu::tex::decompress);
}
TEST_CASE("resize")
{
    test_expected(u8"decompress", btu::tex::decompress);
}
TEST_CASE("optimal_mip_count")
{
    // TODO
}
TEST_CASE("compute_resize_dimension")
{
    // TODO
}
