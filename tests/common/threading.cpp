#include "btu/common/threading.hpp"

#include <catch.hpp>

#include <atomic>
#include <chrono>
#include <thread>

TEST_CASE("for_each_mt", "[src]")
{
    using btu::common::for_each_mt;
    SECTION("exception safe")
    {
        std::vector<int> v(1000);
        CHECK_THROWS(for_each_mt(v, [](auto &&) { throw std::runtime_error("parallel failure"); }));
    }
    SECTION("captures overlapping exceptions")
    {
        std::vector<int> v(10000);
        std::atomic_uint started = 0;
        CHECK_THROWS(for_each_mt(v, [&started](auto &&) {
            if (started.fetch_add(1, std::memory_order_relaxed) == 0)
            {
                const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
                while (started.load(std::memory_order_relaxed) == 1
                       && std::chrono::steady_clock::now() < deadline)
                    std::this_thread::yield();
            }
            throw std::runtime_error("overlapping failure");
        }));
    }
    SECTION("keeps lvalue/rvalue")
    {
        std::vector<std::string> v(100);
        for_each_mt(v, [](std::string &) {});
        for_each_mt(std::move(v), [](std::string &&) {});
    }
}

TEST_CASE("make_producer_mt", "[src]")
{
    using btu::common::make_producer_mt;

    const auto input = std::vector<std::string>(100);

    SECTION("keeps lvalue/rvalue")
    {
        auto collection = input;
        auto ignored    = make_producer_mt<int>(collection, [](std::string &) { return 0; });
        auto ignored2   = make_producer_mt<int>(std::move(collection), [](std::string &&) { return 0; });

        using Range = decltype(std::move(collection));

        using Value = std::conditional_t<std::is_rvalue_reference_v<Range>,
                                         std::ranges::range_value_t<Range> &&,
                                         std::ranges::range_reference_t<Range>>;

        static_assert(std::is_same_v<Value, std::string &&>);
    }

    SECTION("works")
    {
        auto collection         = input;
        auto [thread, receiver] = make_producer_mt<int>(collection, [](std::string &) {
            static std::atomic i = 0;
            return i++;
        });

        auto result = std::set<int>{};

        for (const int i : receiver)
            result.insert(i);

        CHECK(result.size() == input.size());
    }
}
