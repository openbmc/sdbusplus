#include <sdbusplus/event.hpp>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;

TEST(Event, TimeoutWorks)
{
    sdbusplus::event_t e;
    static constexpr auto timeout = 250ms;

    auto start = std::chrono::steady_clock::now();
    e.run_one(timeout);
    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(stop - start > timeout);
    EXPECT_TRUE(stop - start < timeout * 2);
}

TEST(Event, Runnable)
{
    sdbusplus::event_t e;
    static constexpr auto timeout = 10s;

    std::jthread j{[&]() { e.break_run(); }};

    auto start = std::chrono::steady_clock::now();
    e.run_one(timeout);
    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(stop - start < timeout);
}

TEST(Event, ConditionSignals)
{
    sdbusplus::event_t e;

    struct run
    {
        static int _(sd_event_source*, int, uint32_t, void* data)
        {
            *static_cast<bool*>(data) = true;
            return 0;
        }
    };
    bool ran = false;

    auto c = e.add_condition(run::_, &ran);
    std::jthread j{[&]() { c.signal(); }};

    e.run_one();
    EXPECT_TRUE(ran);
    c.ack();
}
