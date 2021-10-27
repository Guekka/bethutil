#pragma once

#include <tl/expected.hpp>

#include <system_error>

namespace DirectX { // NOLINT
class ScratchImage;
class TexMetadata;
} // namespace DirectX

namespace btu::tex {
using DirectX::ScratchImage;
using DirectX::TexMetadata; // NOLINT Not actually unused

using Result = tl::expected<ScratchImage, std::error_code>;

enum class CommandState
{
    Applicable,
    NotRequired,
    NotApplicable
};

} // namespace btu::tex
