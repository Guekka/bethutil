#pragma once

#include "btu/tex/crunch_texture.hpp"
#include "btu/tex/detail/common.hpp"
#include "btu/tex/detail/formats_string.hpp"
#include "btu/tex/dimension.hpp"

#include <crunch/crn_dxt_image.h>
#include <crunch/crn_texture_conversion.h>
#include <crunch/dds_defs.h>

namespace btu::tex {
using crnlib::texture_conversion::convert_params;
[[nodiscard]] auto resize(CrunchTexture &&file, Dimension dim) -> ResultCrunch;
[[nodiscard]] auto generate_mipmaps(CrunchTexture &&file) -> ResultCrunch;
[[nodiscard]] auto convert(CrunchTexture &&file, DXGI_FORMAT format) -> ResultCrunch;
} // namespace btu::tex
