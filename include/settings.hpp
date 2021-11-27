#pragma once

#include "btu/tex/detail/TextureFormats.hpp"

#include <vector>

namespace btu::tex {
enum TextureResizingMode
{
    None,
    ByRatio,
    BySize
};

class Settings
{
public:
    bool bTexturesNecessary;
    bool bTexturesCompress;
    bool bTexturesMipmaps;
    bool bTexturesLandscapeAlpha;

    bool bTexturesResizeMinimum;
    int iTexturesMinimumWidth;
    int iTexturesMinimumHeight;

    TextureResizingMode eTexturesResizingMode;

    int iTexturesResizingHeight;
    int iTexturesResizingWidth;

    DXGI_FORMAT eTexturesFormat;
    bool bTexturesForceConvert;
    std::vector<DXGI_FORMAT> slTextureUnwantedFormats;
};

} // namespace btu::tex
