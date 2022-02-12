#include "btu/nif/functions.hpp"

#include "btu/common/string.hpp"

#include <flow.hpp>
#include <nifly/NifFile.hpp>

namespace btu::nif {
auto get_niversion(btu::common::Game game) -> std::optional<nifly::NiVersion>
{
    using NiVer = nifly::NiVersion;
    switch (game)
    {
        case btu::common::Game::TES3: return std::nullopt;
        case btu::common::Game::TES4: return NiVer::getOB();
        case btu::common::Game::FNV: return NiVer::getFO3();
        case btu::common::Game::SLE: return NiVer::getSK();
        case btu::common::Game::SSE: return NiVer::getSSE();
        case btu::common::Game::FO4: return NiVer::getFO4();
        case btu::common::Game::Custom: return std::nullopt;
    }
    return std::nullopt;
}

auto convert(Mesh &file, bool headpart, btu::common::Game game) -> ResultError
{
    auto target_ver = get_niversion(game);
    if (!target_ver)
        return tl::make_unexpected(btu::common::Error(std::error_code(1, std::generic_category())));
    nifly::OptOptions optOptions{.targetVersion  = *std::move(target_ver),
                                 .headParts      = headpart,
                                 .removeParallax = false};

    file.get().OptimizeFor(optOptions);
    return {};
}

void rename_referenced_textures(Mesh &file)
{
    flow::from(file.get().GetShapes())
        .flat_map([&](auto *s) { return file.get().GetTexturePathRefs(s); })
        .filter([](auto &tex) { return tex.get().size() >= 4; }) // Enough for ".tga"
        .for_each([&](auto &tex) {
            auto ext = btu::common::as_utf8(tex.get()).substr(tex.get().size() - 4);
            if (btu::common::str_compare(ext, u8".tga", false))
                tex.get().replace(tex.get().size() - 4, 4, ".dds");
        });
}
} // namespace btu::nif
