#pragma once

#include <sdbusplus/bus.hpp>

#include <chrono>


namespace sdbusplus
{

namespace test
{

namespace integration
{

class Timer
{
    public:
        Timer():
        start(std::chrono::steady_clock::now())
        {
        };
        
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
