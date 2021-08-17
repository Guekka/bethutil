#pragma once

#include "btu/common/games.hpp"
#include "btu/common/path.hpp"

#ifdef _MSC_VER // Only implementation that uses wchar for path::string_type
#define bethutil_bsa_STR(x) L##x
#else
#define bethutil_bsa_STR(x) x
#endif

namespace btu::bsa {
namespace fs = std::filesystem;
using btu::common::Game;
using btu::common::OsChar;
using btu::common::OsString;
using btu::common::Path;
} // namespace btu::bsa
