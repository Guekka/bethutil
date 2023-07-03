#include "../utils.hpp"
#include "btu/common/filesystem.hpp"
#include "btu/nif/functions.hpp"
#include "btu/nif/mesh.hpp"

#pragma once

inline auto load_nif(const Path &path) -> btu::nif::Mesh
{
    auto res = btu::nif::load(path);
    REQUIRE(res);
    return *res;
}

enum class Approve
{
    Yes,
    No,
};

inline auto test_expected(const Path &root,
                          const Path &filename,
                          const std::function<tl::expected<btu::nif::Mesh, btu::nif::Error>(btu::nif::Mesh)> &f,
                          Approve approve = Approve::No)
{
    auto in_p      = root / "in" / filename;
    auto in        = load_nif(in_p);
    const auto out = f(std::move(in));

    REQUIRE(out);

    const auto expected_path = root / "expected" / filename;
    if (!std::filesystem::exists(expected_path) && approve == Approve::Yes)
    {
        std::filesystem::create_directories(expected_path.parent_path());
        CHECK(btu::nif::save(out.value(), expected_path));
        FAIL_CHECK("Expected file not found:" + expected_path.string());
    }
    else
    {
        REQUIRE(btu::nif::save(out.value(), "out.nif"));
        CHECK(btu::common::compare_files("out.nif", expected_path));
    }
}

template<typename Func>
auto test_expected_dir(const Path &root, const Func &f) -> void
{
    const auto in_dir = root / "in";
    for (const auto &file : std::filesystem::recursive_directory_iterator(in_dir))
        if (file.is_regular_file())
            test_expected(root, file.path().lexically_relative(in_dir), f);
}
