#include <sdbusplus/event.hpp>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;

struct Event : public testing::Test
{
    sdbusplus::event_t ev{};
};

TEST_F(Event, TimeoutWorks)
{
    static constexpr auto timeout = 250ms;

    auto start = std::chrono::steady_clock::now();
    ev.run_one(timeout);
    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(stop - start > timeout);
    EXPECT_TRUE(stop - start < timeout * 3);
}

TEST_F(Event, Runnable)
{
    static constexpr auto timeout = 10s;

    std::jthread j{[&]() { ev.break_run(); }};

    auto start = std::chrono::steady_clock::now();
    ev.run_one(timeout);
    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(stop - start < timeout);
}

TEST_F(Event, ConditionSignals)
{
    struct run
    {
        static int _(sd_event_source*, int, uint32_t, void* data)
        {
            *static_cast<bool*>(data) = true;
            return 0;
        }
    };
    bool ran = false;

    auto c = ev.add_condition(run::_, &ran);
    std::jthread j{[&]() { c.signal(); }};

    ev.run_one();
    EXPECT_TRUE(ran);
    c.ack();
}

TEST_F(Event, Timer)
{
    static constexpr auto timeout = 50ms;

    struct handler
    {
        static int _(sd_event_source*, uint64_t, void* data)
        {
            *static_cast<bool*>(data) = true;
            return 0;
        }
    };
    bool ran = false;

    auto start = std::chrono::steady_clock::now();
    auto c = ev.add_oneshot_timer(handler::_, &ran, timeout);
    ev.run_one();
    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(ran);
    EXPECT_TRUE(stop - start > timeout);
    EXPECT_TRUE(stop - start < timeout * 3);
}
