#pragma once

#include "btu/tex/error_code.hpp"

#include <tl/expected.hpp>


namespace btu::tex {
class Texture;

using Result      = tl::expected<Texture, Error>;
using ResultError = tl::expected<std::monostate, Error>;

enum class CommandState
{
    Applicable,
    NotRequired,
    NotApplicable
};

} // namespace btu::tex
