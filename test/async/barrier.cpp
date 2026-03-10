#include <sdbusplus/async/barrier.hpp>
#include <sdbusplus/async/context.hpp>

#include <gtest/gtest.h>

TEST(Barrier, OneTask)
{
    sdbusplus::async::context ctx;
    sdbusplus::async::barrier b(1);
    bool task_done = false;

    ctx.spawn(b.wait() | sdbusplus::async::execution::then([&]() {
                  task_done = true;
                  ctx.request_stop();
              }));

    ctx.run();

    EXPECT_TRUE(task_done);
}

TEST(Barrier, TwoTasks)
{
    sdbusplus::async::context ctx;
    sdbusplus::async::barrier b(2);
    bool task1_done = false;
    bool task2_done = false;

    ctx.spawn(b.wait() |
              sdbusplus::async::execution::then([&]() { task1_done = true; }));
    ctx.spawn(b.wait() | sdbusplus::async::execution::then([&]() {
                  task2_done = true;
                  ctx.request_stop();
              }));

    ctx.run();

    EXPECT_TRUE(task1_done);
    EXPECT_TRUE(task2_done);
}

TEST(Barrier, ManyTasks)
{
    sdbusplus::async::context ctx;
    constexpr size_t iterations = 100;
    sdbusplus::async::barrier b(iterations);
    int completed_tasks = 0;

    auto task = [&]() {
        return b.wait() | sdbusplus::async::execution::then([&]() {
                   completed_tasks++;
                   if (completed_tasks == iterations)
                   {
                       ctx.request_stop();
                   }
               });
    };

    for (size_t i = 0; i < iterations; ++i)
    {
        ctx.spawn(task());
    }

    ctx.run();

    EXPECT_EQ(completed_tasks, iterations);
}

TEST(Barrier, InvalidCount)
{
    EXPECT_THROW(sdbusplus::async::barrier b(0), std::invalid_argument);
}
