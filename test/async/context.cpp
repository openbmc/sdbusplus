#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

TEST(Context, RunSimple)
{
    sdbusplus::async::context ctx;
    ctx.run(std::execution::just() |
            std::execution::then([&ctx]() { ctx.request_stop(); }));
}

TEST(Context, SpawnedTask)
{
    sdbusplus::async::context ctx;

    ctx.spawn(std::execution::just());

    ctx.run(std::execution::just() |
            std::execution::then([&ctx]() { ctx.request_stop(); }));
}
