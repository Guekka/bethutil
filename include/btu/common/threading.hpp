#pragma once

#include <btu/common/functional.hpp>

namespace btu::common {
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
[[nodiscard]] auto make_producer_mt(Range &&rng, Func &&func)
    requires std::ranges::input_range<Range> && invocable_l_or_r<Func, std::ranges::range_value_t<Range>>
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
} // namespace btu::common