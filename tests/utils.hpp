#include "btu/common/error.hpp"
#include "btu/common/path.hpp"

#include <catch.hpp>
#include <tl/expected.hpp>

#include <fstream>

#ifdef __unix__
constexpr std::string_view k_fail_on_linux_tag = "[!nonportable] [!shouldfail]";
#else
constexpr std::string_view k_fail_on_linux_tag = "[!nonportable]";
#endif

using btu::Path;               // NOLINT(google-global-names-in-headers)
using namespace std::literals; // NOLINT(google-global-names-in-headers)

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
    static auto convert(const btu::Path &in) -> std::string
    {
        return btu::common::as_ascii_string(in.u8string());
    }
};

template<>
struct StringMaker<std::u8string_view>
{
    static auto convert(const std::u8string_view &v) -> std::string
    {
        namespace bc = btu::common;
        using bc::UTF8Iterator, bc::U8Unit;
        return std::string(bc::as_ascii(v)) + " ("
               + StringMaker<std::vector<U8Unit>>::convert(std::vector(UTF8Iterator(v), UTF8Iterator::end(v)))
               + ")";
    }
};

template<>
struct StringMaker<std::u8string>
{
    static auto convert(const std::u8string &v) -> std::string
    {
        return StringMaker<std::u8string_view>::convert(v);
    }
};
} // namespace Catch
