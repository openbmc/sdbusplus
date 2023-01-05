#include <sdbusplus/async.hpp>

#include <chrono>

#include <gtest/gtest.h>

using namespace std::literals;

TEST(Timer, DelaySome)
{
    static constexpr auto timeout = 500ms;

    sdbusplus::async::context ctx;

    auto start = std::chrono::steady_clock::now();

    ctx.spawn(sdbusplus::async::sleep_for(ctx, timeout) |
              stdexec::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();

    auto stop = std::chrono::steady_clock::now();

    EXPECT_GT(stop - start, timeout);
    EXPECT_LT(stop - start, timeout * 3);
}
