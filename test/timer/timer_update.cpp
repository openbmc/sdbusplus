#include "suite.hpp"

#include <sdbusplus/timer.hpp>

#include <chrono>

#include <gtest/gtest.h>

/** @brief Makes sure that timer value is changed in between
 *  and that the new timer expires
 */
TEST_F(TimerTest, updateTimerAndExpectExpire)
{
    using namespace std::chrono;

    auto time = duration_cast<microseconds>(seconds(2));
    EXPECT_GE(timer.start(time), 0);

    // Now sleep for a second and then set the new timeout value
    sleep(1);

    // New timeout is 3 seconds from THIS point.
    time = duration_cast<microseconds>(seconds(3));
    EXPECT_GE(timer.start(time), 0);

    // Wait 3 seconds and see that timer is expired
    int count = 0;
    while (count < 3 && !timer.isExpired())
    {
        // Returns -0- on timeout
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if (!sd_event_run(events, sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(true, timer.isExpired());
    EXPECT_EQ(2, count);
}
