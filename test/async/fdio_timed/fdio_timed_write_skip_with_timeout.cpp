#include "suite.hpp"

#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

using namespace std::literals;

TEST_F(FdioTimedTest, TestWriteSkipWithTimeout)
{
    bool ran = false;
    get_ctx().spawn(testFdTimedEvents(ran, testWriteOperation::writeSkip, 1));
    get_ctx().spawn(
        sdbusplus::async::sleep_for(get_ctx(), 2s) |
        sdbusplus::async::execution::then([&]() { get_ctx().request_stop(); }));
    get_ctx().run();
    EXPECT_TRUE(ran);
}
