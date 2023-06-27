/// Wrapper around DirectXTex.h to remove sal.
/// We are not allowed to use reserved identifiers in C++. But it looks like Microsoft doesn't care.
/// At least they provided a macro that disable some of them, but not enough.
/// So without this fix, compilation breaks on GCC.

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

// clang-format off: order matters

// Prevent Windows.h from being included by DirectXTex.h
#define COM_NO_WINDOWS_H // NOLINT(cppcoreguidelines-macro-usage)
#define PAL_STDCPP_COMPAT 1 // NOLINT(cppcoreguidelines-macro-usage)

#include <DirectXTex.h>
#include <btu/tex/detail/windows.hpp>

// check if windows.h was included
#if defined(_INC_WINDOWS) || defined(_WINDOWS_)
#error "Windows.h was included by DirectXTex.h"
#endif

#undef __valid
// clang-format on

#pragma clang diagnostic pop