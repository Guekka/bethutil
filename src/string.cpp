#include "btu/common/string.hpp"

namespace btu::common{
auto as_utf8_string(std::string str) -> std::u8string
{
    return {std::bit_cast<const char8_t *>(str.data()), str.size()};
}

auto as_ascii_string(std::u8string str) -> std::string
{
    return {std::bit_cast<const char *>(str.data()), str.size()};
}


namespace detail {
thread_local std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};
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
    utf8catcodepoint(string.data() + oldsize, cp, cp_size);
}
}
