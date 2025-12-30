#include "suite.hpp"

#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

using namespace std::literals;

TEST_F(FdioTimedTest, TestWriteSyncIterative)
{
    bool ran = false;
    if (!ctx.has_value())
    {
        GTEST_SKIP() << "ctx empty";
    }
    auto& io = ctx.value();
    io.spawn(testFdTimedEvents(ran, testWriteOperation::writeSync, 100));
    io.spawn(sdbusplus::async::sleep_for(io, 2s) |
             sdbusplus::async::execution::then([&]() { io.request_stop(); }));
    io.run();
    EXPECT_TRUE(ran);
}
