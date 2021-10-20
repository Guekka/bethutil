#include "btu/common/string.hpp"

#include "utf8h/utf8.h"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace btu::common {
void assert_valid_utf8([[maybe_unused]] std::u8string_view str)
{
#ifndef NDEBUG
    if (auto *err = utf8nvalid(str.data(), str.size()))
        throw InvalidUTF8{err};
#endif
}

auto as_utf8(std::string_view str) -> std::u8string_view
{
    static_assert(sizeof(std::string_view::value_type) == sizeof(std::u8string_view::value_type),
                  "btu::common string assumption violated");
    return {reinterpret_cast<const char8_t *>(str.data()), str.size()};
}

auto str_compare(std::u8string_view lhs, std::u8string_view rhs, bool case_sensitive) -> bool
{
    if (lhs.size() != rhs.size())
        return false;

    assert_valid_utf8(lhs);
    assert_valid_utf8(rhs);

    auto f = case_sensitive ? utf8ncasecmp : utf8ncmp;
    return static_cast<bool>(f(lhs.data(), rhs.data(), lhs.size()));
}

auto str_find(std::u8string_view string, std::u8string_view snippet, bool case_sensitive) -> size_t
{
    using std::cbegin, std::cend;

    auto f    = case_sensitive ? utf8casestr : utf8str;
    auto *ptr = f(string.data(), snippet.data());

    return reinterpret_cast<const char8_t *>(ptr) - string.data();
}

auto str_contain(std::u8string_view string, std::u8string_view snippet, bool caseSensitive) -> bool
{
    return str_find(string, snippet, caseSensitive) != std::string::npos;
}

auto to_lower(std::u8string_view str) -> std::u8string
{
    return to_lower(std::u8string(str));
}

auto to_lower(std::u8string &&str) -> std::u8string
{
    auto res = str;
    utf8lwr(res.data());
    return res;
}

} // namespace btu::common
