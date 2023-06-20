/// Wrapper around DirectXTex.h to remove sal.
/// We are not allowed to use reserved identifiers in C++. But it looks like Microsoft doesn't care.
/// At least they provided a macro that disable some of them, but not enough.
/// So without this fix, compilation breaks on GCC.

// clang-format off: order matters

// Windows.h is included by DirectXTex.h
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define NODRAWTEXT
#define NOMCX
#define NOSERVICE
#define NOHELP

#define PAL_STDCPP_COMPAT 1

#include <DirectXTex.h>
#undef __valid
// clang-format on
