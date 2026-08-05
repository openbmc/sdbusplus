#include <sdbusplus/async.hpp>

#include <chrono>
#include <cstdlib>

#include <gtest/gtest.h>

using namespace std::literals;

static bool isValgrind()
{
    static const bool rc = std::getenv("VALGRIND_LIB") != nullptr;
    return rc;
}

TEST(Timer, DelaySome)
{
    static constexpr auto timeout = 500ms;

    sdbusplus::async::context ctx;

    auto start = std::chrono::steady_clock::now();

    ctx.spawn(sdbusplus::async::sleep_for(ctx, timeout) |
              stdexec::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();

    auto stop = std::chrono::steady_clock::now();

    const auto tolerance = isValgrind() ? 16 : 3;

    EXPECT_GT(stop - start, timeout);
    EXPECT_LT(stop - start, timeout * tolerance);
}
