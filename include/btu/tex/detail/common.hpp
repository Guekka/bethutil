#pragma once

#include "btu/tex/error_code.hpp"

#include <tl/expected.hpp>

namespace DirectX { // NOLINT
class ScratchImage;
class TexMetadata;
} // namespace DirectX

namespace btu::tex {
using DirectX::ScratchImage;
using DirectX::TexMetadata; // NOLINT Not actually unused

using Result = tl::expected<ScratchImage, Error>;

enum class CommandState
{
    Applicable,
    NotRequired,
    NotApplicable
};

} // namespace btu::tex
