#include "suite.hpp"

#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

using namespace std::literals;

TEST_F(FdioTimedTest, TestWriteAsyncWithTimeout)
{
    bool ran = false;
    ctx->spawn(testFdTimedEvents(ran, testWriteOperation::writeAsync, 1));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 2s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}
