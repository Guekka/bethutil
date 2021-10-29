#include "btu/common/string.hpp"

#include "utf8h/utf8.h"

#include <algorithm>
#include <cassert>
#include <codecvt>
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

auto UTF8Iterator::operator==(const UTF8Iterator &other) const -> bool
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

static_assert(sizeof(std::string_view::value_type) == sizeof(std::u8string_view::value_type)
                  && sizeof(std::string::value_type) == sizeof(std::u8string::value_type),
              "btu::common string assumption violated");

auto as_utf8(std::string_view str) -> std::u8string_view
{
    return {reinterpret_cast<const char8_t *>(str.data()), str.size()};
}

auto as_ascii(std::u8string_view str) -> std::string_view
{
    return {reinterpret_cast<const char *>(str.data()), str.size()};
}

auto as_utf8_string(std::string str) -> std::u8string
{
    return {reinterpret_cast<const char8_t *>(str.data()), str.size()};
}

auto as_ascii_string(std::u8string str) -> std::string
{
    return {reinterpret_cast<const char *>(str.data()), str.size()};
}

namespace detail {
static thread_local std::wstring_convert<std::codecvt_utf8<wchar_t>> converter{};
} // namespace detail

auto to_utf8(const std::wstring &str) -> std::u8string
{
    return as_utf8_string(detail::converter.to_bytes(str));
}

auto to_utf16(const std::u8string &str) -> std::wstring
{
    return detail::converter.from_bytes(as_ascii_string(str));
}

auto str_compare(std::u8string_view lhs, std::u8string_view rhs, bool case_sensitive) -> bool
{
    if (lhs.size() != rhs.size())
        return false;

    assert_valid_utf8(lhs);
    assert_valid_utf8(rhs);

    auto f = case_sensitive ? utf8ncmp : utf8ncasecmp;
    return f(lhs.data(), rhs.data(), lhs.size()) == 0;
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

auto str_match(std::u8string_view string, std::u8string_view pattern, bool case_sensitive, Cards cards)
    -> bool
{
    // Empty pattern can only match with empty sting
    if (pattern.empty())
        return string.empty();

    auto pat_it        = UTF8Iterator(pattern);
    const auto pat_end = UTF8Iterator::end(pattern);

    auto str_it        = UTF8Iterator(string);
    const auto str_end = UTF8Iterator::end(string);

    auto anyrep_pos_pat = pat_end;
    auto anyrep_pos_str = str_end;

    auto set_pos_pat = pat_end;

    while (str_it != str_end)
    {
        U8Unit current_pat = 0;
        U8Unit current_str = -1;
        if (pat_it != pat_end)
        {
            current_pat = case_sensitive ? *pat_it : utf8lwrcodepoint(*pat_it);
            current_str = case_sensitive ? *str_it : utf8lwrcodepoint(*str_it);
        }
        if (pat_it != pat_end && current_pat == cards.set_begin)
        {
            set_pos_pat = pat_it;
            ++pat_it;
        }
        else if (pat_it != pat_end && current_pat == cards.set_end)
        {
            if (anyrep_pos_pat != pat_end)
            {
                set_pos_pat = pat_end;
                pat_it++;
            }
            else
            {
                return false;
            }
        }
        else if (set_pos_pat != pat_end)
        {
            if (current_pat == current_str)
            {
                set_pos_pat = pat_end;
                pat_it      = std::next(std::find(pat_it, pat_end, cards.set_end));
                ++str_it;
            }
            else
            {
                if (pat_it == pat_end)
                {
                    return false;
                }
                ++pat_it;
            }
        }
        else if (pat_it != pat_end && (current_pat == current_str || current_pat == cards.any))
        {
            ++pat_it;
            ++str_it;
        }
        else if (pat_it != pat_end && current_pat == cards.any_repeat)
        {
            anyrep_pos_pat = pat_it;
            anyrep_pos_str = str_it;
            ++pat_it;
        }
        else if (anyrep_pos_pat != pat_end)
        {
            pat_it = std::next(anyrep_pos_pat);
            str_it = std::next(anyrep_pos_str);
            ++anyrep_pos_str;
        }
        else
        {
            return false;
        }
    }
    while (pat_it != pat_end)
    {
        const U8Unit cur = case_sensitive ? *pat_it : utf8lwrcodepoint(*pat_it);
        if (cur == cards.any_repeat)
            ++pat_it;
        else
            break;
    }
    return pat_it == pat_end;
}
} // namespace btu::common
