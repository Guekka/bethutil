#pragma once

#include <filesystem>

#ifdef _MSC_VER // Only implementation that uses wchar for path::string_type
#define bethutil_bsa_STR(x) L##x
#else
#define bethutil_bsa_STR(x) x
#endif

namespace btu::bsa {
namespace fs = std::filesystem;
using fs::path;
using string = path::string_type;
using charT  = string::value_type;
} // namespace BethUtil::BSA
