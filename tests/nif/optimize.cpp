#include "./utils.hpp"

#include <btu/nif/optimize.hpp>

TEST_CASE("nif_optimize", "[src]")
{
    const Path dir = "nif_optimize";

    auto tester = [&](auto &&mesh) {
        const auto opt_sets  = btu::nif::Settings::get(btu::Game::SSE);
        const auto opt_steps = btu::nif::compute_optimization_steps(mesh, opt_sets);

        REQUIRE(opt_steps.rename_referenced_textures);
        REQUIRE(opt_steps.format == btu::Game::SSE);
        REQUIRE(opt_steps.headpart == false);

        return btu::nif::optimize(BTU_FWD(mesh), opt_steps);
    };

    test_expected(dir, "crashing.nif", tester);
}

TEST_CASE("detect headpart", "[src]")
{
    auto mesh = btu::nif::Mesh{};

    const auto opt_sets = btu::nif::Settings::get(btu::Game::SSE);
    mesh.set_load_path(opt_sets.headpart_meshes[0]);

    REQUIRE(btu::nif::compute_optimization_steps(mesh, opt_sets).headpart);
}