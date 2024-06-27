#include "btu/nif/functions.hpp"

#include "btu/common/string.hpp"

#include <flux.hpp>
#include <nifly/NifFile.hpp>

namespace btu::nif {
auto get_niversion(Game game) -> std::optional<nifly::NiVersion>
{
    using NiVer = nifly::NiVersion;
    switch (game)
    {
        case Game::TES3: return std::nullopt;
        case Game::TES4: return NiVer::getOB();
        case Game::FNV: return NiVer::getFO3();
        case Game::SLE: return NiVer::getSK();
        case Game::SSE: return NiVer::getSSE();
        case Game::FO4: return NiVer::getFO4();
        case Game::Starfield: return NiVer::getSF();
        case Game::Custom: return std::nullopt;
    }
    return std::nullopt;
}

auto convert(Mesh file, HeadpartStatus headpart, Game game) -> tl::expected<Mesh, Error>
{
    auto target_ver = get_niversion(game);
    if (!target_ver)
        return tl::make_unexpected(Error(std::error_code(1, std::generic_category())));
    nifly::OptOptions opt_options{.targetVersion  = *std::move(target_ver),
                                  .headParts      = headpart == HeadpartStatus::Yes,
                                  .removeParallax = false};

    file.get().OptimizeFor(opt_options);
    return file;
}

void rename_referenced_textures(Mesh &file)
{
    flux::from(file.get().GetShapes())
        .map([&](auto *s) { return file.get().GetTexturePathRefs(s); })
        .flatten()
        .filter([](auto &tex) { return tex.get().size() >= 4; }) // Enough for ".tga"
        .for_each([&](auto &tex) {
            auto ext = common::as_utf8(tex.get()).substr(tex.get().size() - 4);
            if (common::str_compare(ext, u8".tga", common::CaseSensitive::No))
                tex.get().replace(tex.get().size() - 4, 4, ".dds");
        });
}
} // namespace btu::nif
