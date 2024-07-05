#pragma once

#include <BS_thread_pool.hpp>
#include <btu/common/functional.hpp>
#include <btu/common/metaprogramming.hpp>

#include <span>

namespace btu::common {
using ThreadPool = BS::thread_pool;

// GCOVR_EXCL_START : this code comes from <https://github.com/jrgfogh/synchronized_value> and seems well tested
// NOLINTBEGIN
template<typename L>
concept basic_lockable = requires(L m) {
    m.lock();
    m.unlock();
};

template<typename GuardedType, basic_lockable MutexType = std::mutex>
class synchronized;

template<typename GuardedType, basic_lockable MutexType = std::mutex>
class update_guard
{
    std::unique_lock<MutexType> lock_;
    GuardedType *guarded_data_;

public:
    using value_type = GuardedType;
    using mutex_type = MutexType;

    explicit update_guard(synchronized<GuardedType, MutexType> &sv)
        : lock_{sv.mutex_}
        , guarded_data_{&sv.guarded_data_}
    {
    }

    explicit update_guard(const synchronized<std::remove_const_t<GuardedType>, MutexType> &sv)
        : lock_{sv.mutex_}
        , guarded_data_{&sv.guarded_data_}
    {
    }

    explicit update_guard(synchronized<GuardedType, MutexType> &sv, std::unique_lock<MutexType> lock)
        : lock_{std::move(lock)}
        , guarded_data_{&sv.guarded_data_}
    {
    }

    explicit update_guard(const synchronized<std::remove_const_t<GuardedType>, MutexType> &sv,
                          std::unique_lock<MutexType> lock)
        : lock_{std::move(lock)}
        , guarded_data_{&sv.guarded_data_}
    {
    }

    auto operator->() noexcept -> GuardedType * { return guarded_data_; }
    auto operator*() noexcept -> GuardedType & { return *guarded_data_; }
};

template<typename GuardedData, basic_lockable MutexType>
class synchronized
{
    friend class update_guard<GuardedData, MutexType>;
    friend class update_guard<GuardedData const, MutexType>;

    mutable MutexType mutex_;
    GuardedData guarded_data_;

public:
    using value_type = GuardedData;
    using mutex_type = MutexType;

    synchronized(synchronized const &)                     = delete;
    auto operator=(synchronized const &) -> synchronized & = delete;

    template<typename... Args>
    explicit synchronized(Args &&...args)
        : guarded_data_{std::forward<Args>(args)...}
    {
    }

    template<typename F>
    auto apply(F const &func)
    {
        std::unique_lock lock{mutex_};
        return func(guarded_data_);
    }

    [[nodiscard]] auto wlock() { return update_guard{*this}; }

    [[nodiscard]] auto rlock() const { return update_guard<GuardedData const, MutexType>{*this}; }

    // The two next functions are mine, hope I did nothing wrong
    [[nodiscard]] auto try_rlock() const -> std::optional<update_guard<GuardedData const, MutexType>>
    {
        std::unique_lock lock{mutex_, std::try_to_lock};
        if (lock.owns_lock())
            return update_guard<GuardedData const, MutexType>{*this, std::move(lock)};
        return std::nullopt;
    }

    [[nodiscard]] auto try_wlock() -> std::optional<update_guard<GuardedData, MutexType>>
    {
        std::unique_lock lock{mutex_, std::try_to_lock};
        if (lock.owns_lock())
            return update_guard<GuardedData, MutexType>{*this, std::move(lock)};
        return std::nullopt;
    }
};

// GCOVR_EXCL_STOP : end of synchronized_value code
// NOLINTEND

[[nodiscard]] inline auto hardware_concurrency() noexcept -> unsigned
{
    const auto result = std::thread::hardware_concurrency();
    return result ? result : 1;
}

/** \brief Creates a thread pool with the number of threads equal to the number of hardware threads minus one.
 *
 * \return The thread pool object.
 */
[[nodiscard]] inline auto make_thread_pool()
{
    static const auto num_threads = std::max(hardware_concurrency() - 1, 1u);
    return ThreadPool{num_threads};
}

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