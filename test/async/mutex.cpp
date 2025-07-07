#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

using namespace std::literals;

class MutexTest : public ::testing::Test
{
  protected:
    constexpr static std::string testMutex = "TestMutex";

    MutexTest() : mutex(*ctx, testMutex) {}

    std::optional<sdbusplus::async::context> ctx{std::in_place};

    auto testAsyncAddition(int val = 1) -> sdbusplus::async::task<>
    {
        EXPECT_EQ(co_await mutex.lock(), true);

        sharedVar += val;

        mutex.unlock();
    }

    auto testAsyncSubtraction(int val = 1) -> sdbusplus::async::task<>
    {
        EXPECT_EQ(co_await mutex.lock(), true);

        sharedVar -= val;

        mutex.unlock();
    }

    int sharedVar = 0;

  private:
    sdbusplus::async::mutex mutex;
};

TEST_F(MutexTest, TestAsyncAddition)
{
    constexpr auto testIterations = 10;
    for (auto i = 0; i < testIterations; i++)
    {
        ctx->spawn(testAsyncAddition());
    }

    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 1s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));

    ctx->run();

    EXPECT_EQ(sharedVar, testIterations);
}

TEST_F(MutexTest, TestAsyncMixed)
{
    constexpr auto testIterations = 10;
    for (auto i = 0; i < testIterations; i++)
    {
        ctx->spawn(testAsyncAddition());
        ctx->spawn(testAsyncSubtraction(2));
    }

    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 1s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));

    ctx->run();

    EXPECT_EQ(sharedVar, -testIterations);
}
