#pragma once

#include <sdbusplus/bus.hpp>

#include <chrono>

namespace sdbusplus
{

namespace test
{

namespace integration
{

/** This is a simple utility class to manage the timer for procedures that
 * should run for a certain amount of time.
 *
 * The timer starts upon construction and each call to the duration function,
 * returns the amount of time that has passed since the start of the timer.
 */
class Timer
{
  public:
    /* Constructs and starts the timer.
     */
    Timer() : start(std::chrono::steady_clock::now()){};

    /* The time that has passed since the timer is started.
     */
    SdBusDuration duration()
    {
        return std::chrono::duration_cast<SdBusDuration>(
            std::chrono::steady_clock::now() - start);
    };

  private:
    std::chrono::steady_clock::time_point start;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
