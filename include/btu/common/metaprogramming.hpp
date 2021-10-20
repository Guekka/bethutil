/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

#pragma once

#include <type_traits>

namespace btu::common {
template<class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overload(Ts...) -> overload<Ts...>;

template<typename E>
constexpr auto to_underlying(E e) -> std::underlying_type_t<E>
{
    return static_cast<std::underlying_type_t<E>>(e);
}

template<typename T, typename U>
using is_equiv = std::is_same<std::remove_cvref_t<T>, std::remove_cvref_t<U>>;

template<typename T, typename U>
constexpr bool is_equiv_v = is_equiv<T, U>::value;

} // namespace btu::common

#define BETHUTIL_MAKE_ENUM_OPERATOR_PAIR(a_type, a_op)                                           \
    [[nodiscard]] constexpr auto operator a_op(a_type a_lhs, a_type a_rhs) noexcept->a_type      \
    {                                                                                            \
        return static_cast<a_type>(static_cast<std::underlying_type_t<a_type>>(a_lhs)            \
                                       a_op static_cast<std::underlying_type_t<a_type>>(a_rhs)); \
    }                                                                                            \
                                                                                                 \
    constexpr auto operator a_op##=(a_type &a_lhs, a_type a_rhs) noexcept->a_type &              \
    {                                                                                            \
        return a_lhs = a_lhs a_op a_rhs;                                                         \
    }

#define BETHUTIL_MAKE_ALL_ENUM_OPERATORS(a_type)                                         \
    static_assert(std::is_enum_v<a_type>, "\'" #a_type "\' is not an enum");             \
                                                                                         \
    [[nodiscard]] constexpr auto operator~(a_type a_val) noexcept->a_type                \
    {                                                                                    \
        return static_cast<a_type>(~static_cast<std::underlying_type_t<a_type>>(a_val)); \
    }                                                                                    \
                                                                                         \
    BETHUTIL_MAKE_ENUM_OPERATOR_PAIR(a_type, ^)                                          \
    BETHUTIL_MAKE_ENUM_OPERATOR_PAIR(a_type, &)                                          \
    BETHUTIL_MAKE_ENUM_OPERATOR_PAIR(a_type, |)                                          \
    BETHUTIL_MAKE_ENUM_OPERATOR_PAIR(a_type, <<)                                         \
    BETHUTIL_MAKE_ENUM_OPERATOR_PAIR(a_type, >>)
