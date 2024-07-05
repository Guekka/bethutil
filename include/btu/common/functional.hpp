/* Copyright (C) 2021 Edgar B
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

// bind_back is from range v3 :
// Range v3 library
//
//  Copyright Andrey Diduh 2019
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#pragma once
#include "metaprogramming.hpp"

#include <mpsc/mpsc_channel.hpp>

#include <exception>
#include <execution>
#include <functional>
#include <iterator>
#include <optional>
#include <thread>
#include <tuple>
#include <utility>

namespace btu::common {
namespace detail {
// bind_back like std::bind_front has no special treatment for nested
// bind-expressions or reference_wrappers; there is no need to wrap
// Callables with ranges::protect.
template<typename Fn, typename... Args>
struct bind_back_fn_
{
    using tuple_t = std::tuple<Fn, Args...>;
    tuple_t fn_args_;

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn, CallArgs..., Args...> //
    operator()(CallArgs &&...cargs) && noexcept(std::is_nothrow_invocable_v<Fn, CallArgs..., Args...>)
    {
        return tuple_apply(
            [&](auto &&fn, auto &&...args) -> decltype(auto) {
                return invoke((decltype(fn)) fn, (CallArgs &&) cargs..., (decltype(args)) args...);
            },
            (std::tuple<Fn, Args...> &&) fn_args_);
    }

    /// \overload
    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn &, CallArgs..., Args &...> operator()(CallArgs &&...cargs) & noexcept(
        std::is_nothrow_invocable_v<Fn &, CallArgs..., Args &...>)
    {
        return tuple_apply([&](auto &fn, auto &...args)
                               -> decltype(auto) { return invoke(fn, (CallArgs &&) cargs..., args...); },
                           fn_args_);
    }

    /// \overload
    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn const &, CallArgs..., Args const &...> operator()(
        CallArgs &&...cargs) const & //
        noexcept(std::is_nothrow_invocable_v<Fn const &, CallArgs..., Args const &...>)
    {
        return tuple_apply([&](auto &fn, auto &...args)
                               -> decltype(auto) { return invoke(fn, (CallArgs &&) cargs..., args...); },
                           fn_args_);
    }
};

/// \cond
// Unroll a few instantiations to avoid a heavy-weight tuple instantiation
template<typename Fn, typename Arg>
struct bind_back_fn_<Fn, Arg>
{
    struct tuple_t
    {
        Fn fn_;
        Arg arg_;
    };
    tuple_t fn_args_;

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn, CallArgs..., Arg> //
    operator()(CallArgs &&...cargs) &&                   //
        noexcept(std::is_nothrow_invocable_v<Fn, CallArgs..., Arg>)
    {
        return invoke((Fn &&) fn_args_.fn_, (CallArgs &&) cargs..., (Arg &&) fn_args_.arg_);
    }

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn &, CallArgs..., Arg &> //
    operator()(CallArgs &&...cargs) &                        //
        noexcept(std::is_nothrow_invocable_v<Fn &, CallArgs..., Arg &>)
    {
        return invoke(fn_args_.fn_, (CallArgs &&) cargs..., fn_args_.arg_);
    }

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn const &, CallArgs..., Arg const &> //
    operator()(CallArgs &&...cargs) const &                              //
        noexcept(std::is_nothrow_invocable_v<Fn const &, CallArgs..., Arg const &>)
    {
        return invoke(fn_args_.fn_, (CallArgs &&) cargs..., fn_args_.arg_);
    }
};

template<typename Fn, typename Arg0, typename Arg1>
struct bind_back_fn_<Fn, Arg0, Arg1>
{
    struct tuple_t
    {
        Fn fn_;
        Arg0 arg0_;
        Arg1 arg1_;
    };
    tuple_t fn_args_;

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn, CallArgs..., Arg0, Arg1> //
    operator()(CallArgs &&...cargs) &&                          //
        noexcept(std::is_nothrow_invocable_v<Fn, CallArgs..., Arg0, Arg1>)
    {
        return invoke((Fn &&) fn_args_.fn_,
                      (CallArgs &&) cargs...,
                      (Arg0 &&) fn_args_.arg0_,
                      (Arg1 &&) fn_args_.arg1_);
    }

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn &, CallArgs..., Arg0 &, Arg1 &> //
    operator()(CallArgs &&...cargs) &                                 //
        noexcept(std::is_nothrow_invocable_v<Fn &, CallArgs..., Arg0 &, Arg1 &>)
    {
        return invoke(fn_args_.fn_, (CallArgs &&) cargs..., fn_args_.arg0_, fn_args_.arg1_);
    }

    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn const &, CallArgs..., Arg0 const &, Arg1 const &> operator()(
        CallArgs &&...cargs)
        const & noexcept(std::is_nothrow_invocable_v<Fn const &, CallArgs..., Arg0 const &, Arg1 const &>)
    {
        return invoke(fn_args_.fn_, (CallArgs &&) cargs..., fn_args_.arg0_, fn_args_.arg1_);
    }
};
/// \endcond

template<typename Fn, typename... Args>
using bind_back_fn2 = bind_back_fn_<std::decay_t<Fn>, std::decay_t<Args>...>;

struct bind_back_fn
{
    template<typename Fn, typename Arg1, typename... Args>
    constexpr bind_back_fn2<Fn, Arg1, Args...> //
    operator()(Fn && fn, Arg1 && arg1, Args &&...args) const
    {
        return {{(Fn &&) fn, (Arg1 &&) arg1, (Args &&) args...}};
    }
};
} // namespace detail

/// \sa `bind_back_fn`
inline constexpr detail::bind_back_fn bind_back{};

template<class Func, class Arg>
concept invocable_l_or_r = std::invocable<Func, Arg &> || std::invocable<Func, Arg &&>;

template<typename T>
class Lazy
{
    std::function<T()> func_;
    mutable std::optional<T> value_;

public:
    explicit Lazy(std::function<T()> func)
        : func_(std::move(func))
    {
    }

    auto operator*() const -> T &
    {
        if (!value_)
            value_ = std::move(func_)();
        return *value_;
    }

    auto operator->() const -> T *
    {
        if (!value_)
            value_ = std::move(func_)();
        return &*value_;
    }
};

#define BTU_RETURNS(...)                                   \
    noexcept(noexcept(__VA_ARGS__))->decltype(__VA_ARGS__) \
    {                                                      \
        return __VA_ARGS__;                                \
    }

#define BTU_RESOLVE_OVERLOAD(f) [](auto &&...xs) BTU_RETURNS(f(::std::forward<decltype(xs)>(xs)...))

} // namespace btu::common
