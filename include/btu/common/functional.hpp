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
#include <iostream>
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
    operator()(CallArgs &&...cargs) &&noexcept(std::is_nothrow_invocable_v<Fn, CallArgs..., Args...>)
    {
        return tuple_apply(
            [&](auto &&fn, auto &&...args) -> decltype(auto) {
                return invoke((decltype(fn)) fn, (CallArgs &&) cargs..., (decltype(args)) args...);
            },
            (std::tuple<Fn, Args...> &&) fn_args_);
    }

    /// \overload
    template<typename... CallArgs>
    constexpr std::invoke_result_t<Fn &, CallArgs..., Args &...> operator()(CallArgs &&...cargs) &noexcept(
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
        const &noexcept(std::is_nothrow_invocable_v<Fn const &, CallArgs..., Arg0 const &, Arg1 const &>)
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
    operator()(Fn &&fn, Arg1 &&arg1, Args &&...args) const
    {
        return {{(Fn &&) fn, (Arg1 &&) arg1, (Args &&) args...}};
    }
};
} // namespace detail

/// \ingroup group-utility
/// \sa `bind_back_fn`
inline constexpr detail::bind_back_fn bind_back{};

template<class Func, class Arg>
concept invocable_l_or_r = std::invocable<Func, Arg &> || std::invocable<Func, Arg &&>;

template<typename Range, typename Func>
requires std::ranges::input_range<Range> && invocable_l_or_r<Func, std::ranges::range_value_t<Range>>
auto for_each_mt(Range &&rng, Func &&func)
{
    auto eptr = std::exception_ptr{};
    auto call = [&eptr, f = std::forward<Func>(func)](auto &&elem) mutable noexcept {
        if (eptr)
            return; // We do not process the remaining elements if an exception has been encountered

        try
        {
            f(std::forward<decltype(elem)>(elem));
        }
        catch (const std::exception &)
        {
            eptr = std::current_exception();
        }
    };

    using std::begin, std::end, std::make_move_iterator;
    if constexpr (std::is_lvalue_reference_v<decltype(rng)>)
        std::for_each(std::execution::par, begin(rng), end(rng), std::move(call));
    else
        std::for_each(std::execution::par,
                      make_move_iterator(begin(rng)),
                      make_move_iterator(end(rng)),
                      std::move(call));

    if (eptr)
        std::rethrow_exception(eptr);
}

/**
 * \brief Creates a multi-threaded producer for the specified range and function.
 *
 * This function creates a multi-threaded producer that generates values by applying the specified function
 * to each element in the given range. The producer distributes the workload across multiple threads to improve
 * performance.
 *
 * \param rng   The range of elements to process.
 * \param func  The function to apply to each element in the range.
 *
 * \tparam Range    The type of the range.
 * \tparam Func     The type of the function.
 *
 * \return The multi-threaded producer object.
 *
 * \note The multi-threaded producer returned by this function is responsible for managing the parallel execution
 *       of the given function on the range elements. The user is responsible for synchronizing access to any shared
 *       data if needed.
 */

template<typename Out, typename Range, typename Func>
[[nodiscard]] auto make_producer_mt(Range &&rng, Func &&func) requires
    std::ranges::input_range<Range> && invocable_l_or_r<Func, std::ranges::range_value_t<Range>>
{
    auto channel = mpsc::Channel<Out>::make();
    auto sender  = std::get<0>(channel);

    auto producer = std::jthread([rng    = std::forward<Range>(rng),
                                  func   = std::forward<Func>(func),
                                  sender = std::move(sender)]() mutable {
        for_each_mt(std::forward<Range>(rng),
                    [func, sender](auto &&elem) mutable { sender.send(func(forward_like<Range>(elem))); });
        sender.close();
    });

    return std::pair{std::move(producer), std::get<1>(std::move(channel))};
}

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

} // namespace btu::common
