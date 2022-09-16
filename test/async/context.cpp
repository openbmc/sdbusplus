#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

TEST(Context, RunSimple)
{
    sdbusplus::async::context ctx;
    ctx.run(std::execution::just() |
            std::execution::then([&ctx]() { ctx.request_stop(); }));
}
