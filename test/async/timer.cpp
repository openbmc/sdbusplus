#include <sdbusplus/async.hpp>

#include <chrono>

#include <gtest/gtest.h>

using namespace std::literals;

TEST(Timer, DelaySome)
{
    static constexpr auto timeout = 10ms;

    sdbusplus::async::context ctx;

    auto start = std::chrono::steady_clock::now();

    ctx.run(sdbusplus::async::sleep_for(ctx, timeout) |
            std::execution::then([&ctx]() { ctx.request_stop(); }));

    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(stop - start > timeout);
    EXPECT_TRUE(stop - start < timeout * 2);
}
