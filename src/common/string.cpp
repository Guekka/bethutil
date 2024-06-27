#include "btu/common/string.hpp"

#include "utf8h/utf8.h"

#include <locale>

namespace btu::common {
auto as_utf8_string(std::string str) -> std::u8string
{
    return {reinterpret_cast<const char8_t *>(str.data()), str.size()};
}

auto as_ascii_string(std::u8string str) -> std::string
{
    return {reinterpret_cast<const char *>(str.data()), str.size()};
}

auto as_utf8(std::string_view str) -> std::u8string_view
{
    return {reinterpret_cast<const char8_t *>(str.data()), str.size()};
}

auto as_ascii(std::u8string_view str) -> std::string_view
{
    return {reinterpret_cast<const char *>(str.data()), str.size()};
}

namespace detail {
// utility wrapper to adapt locale-bound facets for wstring/wbuffer convert
template<class Facet>
struct DeletableFacet : Facet
{
    template<class... Args>
    explicit DeletableFacet(Args &&...args)
        : Facet(std::forward<Args>(args)...)
    {
    }

    DeletableFacet(const DeletableFacet &)                     = delete;
    auto operator=(const DeletableFacet &) -> DeletableFacet & = delete;

    DeletableFacet(DeletableFacet &&)                     = default;
    auto operator=(DeletableFacet &&) -> DeletableFacet & = default;

    ~DeletableFacet() override = default;
};

thread_local std::wstring_convert<DeletableFacet<std::codecvt<wchar_t, char, std::mbstate_t>>> converter{};
} // namespace detail

auto to_utf8(const std::wstring &str) -> std::u8string
{
    return as_utf8_string(detail::converter.to_bytes(str));
}

auto to_utf16(const std::u8string &str) -> std::wstring
{
    return detail::converter.from_bytes(as_ascii_string(str));
}

auto to_lower(std::u8string_view string) -> std::u8string
{
    detail::assert_valid_utf8(string);

    auto res = std::u8string{};
    res.reserve(string.size());

    std::for_each(UTF8Iterator(string), UTF8Iterator::end(string), [&](U8Unit u8) {
        concat_codepoint(res, utf8lwrcodepoint(u8));
    });

    return res;
}

void concat_codepoint(std::u8string &string, U8Unit cp)
{
    const auto oldsize = string.size();
    const auto cp_size = utf8codepointsize(cp);
    string.resize(oldsize + cp_size);
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    utf8catcodepoint(string.data() + oldsize, cp, cp_size);
}

auto make_valid(std::u8string &string, char8_t replacement) noexcept -> std::u8string &
{
    utf8makevalid(string.data(), replacement);
    return string;
}

} // namespace btu::common
