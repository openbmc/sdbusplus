#include "suite.hpp"

#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

using namespace std::literals;

TEST_F(FdioTimedTest, TestWriteSyncIterative)
{
    bool ran = false;
    ASSERT_TRUE(ctx.has_value());
    if (!ctx.has_value())
        return; // structural guard just for the analyzer
    auto& io = ctx.value();
    io.spawn(testFdTimedEvents(ran, testWriteOperation::writeSync, 100));
    io.spawn(sdbusplus::async::sleep_for(io, 2s) |
             sdbusplus::async::execution::then([&]() { io.request_stop(); }));
    io.run();
    EXPECT_TRUE(ran);
}
