#pragma once

#include "btu/common/games.hpp"
#include "btu/common/path.hpp"

#include <fstream>

#ifdef _MSC_VER // Only implementation that uses wchar for path::string_type
#define BETHUTIL_BSA_STR(x) L##x
#else
#define BETHUTIL_BSA_STR(x) x
#endif

namespace btu::bsa {
namespace fs = std::filesystem;
using btu::common::Game;
using btu::common::OsChar;
using btu::common::OsString;
using btu::common::Path;
} // namespace btu::bsa

namespace btu::bsa::detail {
template<class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

[[nodiscard]] inline auto read_file(const std::filesystem::path &a_path) -> std::vector<std::byte>
{
    std::vector<std::byte> data;
    data.resize(std::filesystem::file_size(a_path));

    std::ifstream in{a_path, std::ios_base::in | std::ios_base::binary};
    in.exceptions(std::ios_base::failbit);
    in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));

    return data;
}

template<typename E>
constexpr auto to_underlying(E e) -> std::underlying_type_t<E>
{
    return static_cast<std::underlying_type_t<E>>(e);
}
} // namespace btu::bsa::detail
