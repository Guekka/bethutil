/// Wrapper around DirectXTex.h to remove sal.
/// We are not allowed to use reserved identifiers in C++. But it looks like Microsoft doesn't care.
/// At least they provided a macro that disable some of them, but not enough.
/// So without this fix, compilation breaks on GCC.

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

// clang-format off: order matters

#define PAL_STDCPP_COMPAT 1 // NOLINT(cppcoreguidelines-macro-usage)

#include <DirectXTex.h>
#include <btu/tex/detail/windows.hpp>

#undef __valid
// clang-format on

#pragma clang diagnostic pop