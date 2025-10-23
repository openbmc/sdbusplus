#include "suite.hpp"

#include <sdbusplus/timer.hpp>

#include <chrono>

#include <gtest/gtest.h>

/** @brief Makes sure that timer is not expired
 */
TEST_F(TimerTest, timerNotExpiredAfter2Seconds)
{
    using namespace std::chrono;

    auto time = duration_cast<microseconds>(seconds(2));
    EXPECT_GE(timer.start(time), 0);

    // Now turn off the timer post a 1 second sleep
    sleep(1);
    EXPECT_GE(timer.stop(), 0);

    // Wait 2 seconds and see that timer is not expired
    int count = 0;
    while (count < 2)
    {
        // Returns -0- on timeout
        auto sleepTime = duration_cast<microseconds>(seconds(1));
        if (!sd_event_run(events, sleepTime.count()))
        {
            count++;
        }
    }
    EXPECT_EQ(false, timer.isExpired());

    // 2 because of one more count that happens prior to exiting
    EXPECT_EQ(2, count);
}
