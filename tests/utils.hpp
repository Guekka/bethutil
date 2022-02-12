#include "btu/common/error.hpp"
#include "btu/common/path.hpp"

#include <catch2/catch.hpp>
#include <tl/expected.hpp>

#include <fstream>

using btu::Path;
namespace fs = btu::fs;
using namespace std::literals;

namespace Catch {
template<typename T, typename E>
struct StringMaker<tl::expected<T, E>>
{
    static auto convert(const tl::expected<T, E> &expected) -> std::string
    {
        auto ts = [](auto &&a) { return StringMaker<decltype(a)>::convert(a); };
        return expected.has_value() ? ts(expected.value()) : ts(expected.error());
    }
};

template<typename E>
struct StringMaker<tl::expected<void, E>>
{
    static auto convert(const tl::expected<void, E> &expected) -> std::string
    {
        auto ts = [](auto &&a) { return StringMaker<decltype(a)>::convert(a); };
        return expected.has_value() ? "void value" : ts(expected.error());
    }
};

template<>
struct StringMaker<btu::Path>
{
    static std::string convert(const btu::Path &in) { return in.string(); }
};
} // namespace Catch
