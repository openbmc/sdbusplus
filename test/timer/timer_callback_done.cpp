#include "suite.hpp"

#include <sdbusplus/timer.hpp>

#include <chrono>

#include <gtest/gtest.h>

/** @brief Makes sure that optional callback is called */
TEST_F(TimerTestCallBack, optionalFuncCallBackDone)
{
    using namespace std::chrono;

    auto time = duration_cast<microseconds>(seconds(2));
    EXPECT_GE(timer->start(time), 0);

    // Waiting 2 seconds is enough here since we have
    // already spent some usec now
    int count = 0;
    while (count < 2 && !timer->isExpired())
    {
        // Returns -0- on timeout and positive number on dispatch
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if (!sd_event_run(events, sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, timer->isExpired());
    EXPECT_EQ(true, callBackDone);
    EXPECT_EQ(1, count);
}
