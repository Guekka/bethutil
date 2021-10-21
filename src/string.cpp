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
        throw InvalidUTF8{};
#endif
}

UTF8Iterator::UTF8Iterator(std::u8string_view string)
    : string_(string)
{
    if (string_.empty())
        return;

    assert_valid_utf8(string_);
    utf8codepoint(string_.data(), &cur_);
}

auto UTF8Iterator::end(std::u8string_view string) -> UTF8Iterator
{
    UTF8Iterator it(string);
    while (it.idx_ < it.string_.size())
        ++it;
    return it;
}

auto UTF8Iterator::operator*() const -> UTF8Iterator::value_type
{
    return cur_;
}

auto UTF8Iterator::operator->() const -> UTF8Iterator::pointer
{
    return cur_;
}

auto UTF8Iterator::operator++() -> UTF8Iterator &
{
    idx_ += utf8codepointsize(cur_);
    utf8codepoint(string_.data() + idx_, &cur_);
    return *this;
}

bool UTF8Iterator::operator==(const UTF8Iterator &other) const
{
    return (*this <=> other) == 0;
}

auto UTF8Iterator::operator<=>(const UTF8Iterator &other) const -> std::strong_ordering
{
    if (idx_ != other.idx_)
        return idx_ <=> other.idx_;

    if (cur_ != other.cur_)
        return cur_ <=> other.cur_;

    return string_.data() <=> other.string_.data();
}

auto UTF8Iterator::operator++(int) -> UTF8Iterator
{
    auto copy = *this;
    ++*this;
    return copy;
}

auto as_utf8(std::string_view str) -> std::u8string_view
{
    static_assert(sizeof(std::string_view::value_type) == sizeof(std::u8string_view::value_type),
                  "btu::common string assumption violated");
    return {reinterpret_cast<const char8_t *>(str.data()), str.size()};
}

auto as_ascii(std::u8string_view str) -> std::string_view
{
    static_assert(sizeof(std::string_view::value_type) == sizeof(std::u8string_view::value_type),
                  "btu::common string assumption violated");
    return {reinterpret_cast<const char *>(str.data()), str.size()};
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
    assert_valid_utf8(string);
    assert_valid_utf8(snippet);

    using std::cbegin, std::cend;

    auto f    = case_sensitive ? utf8str : utf8casestr;
    auto *ptr = f(string.data(), snippet.data());

    if (ptr == nullptr)
        return std::u8string::npos;

    return reinterpret_cast<const char8_t *>(ptr) - string.data();
}

auto str_contain(std::u8string_view string, std::u8string_view snippet, bool case_sensitive) -> bool
{
    return str_find(string, snippet, case_sensitive) != std::string::npos;
}

auto to_lower(std::u8string_view string) -> std::u8string
{
    assert_valid_utf8(string);

    auto res = std::u8string{};

    res = string;
    utf8lwr(res.data());
    return res;

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

auto first_codepoint(std::u8string_view string) -> U8Unit
{
    U8Unit res{};
    utf8codepoint(string.data(), &res);
    return res;
}

} // namespace btu::common
