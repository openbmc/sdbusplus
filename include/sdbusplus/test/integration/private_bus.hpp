#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/dbusd_manager.hpp>

#include <memory>

namespace sdbusplus
{

namespace test
{

namespace integration
{

class PrivateBus
{
  public:
    PrivateBus(const PrivateBus&) = delete;
    PrivateBus& operator=(const PrivateBus&) = delete;
    PrivateBus(PrivateBus&&) = delete;
    PrivateBus& operator=(PrivateBus&&) = delete;

    PrivateBus();

    std::shared_ptr<sdbusplus::bus::bus> getBus();

    void runFor(SdBusDuration microseconds);

  private:
    DBusDaemon daemon;

    std::shared_ptr<sdbusplus::bus::bus> bus;

    static const int defaultWarmupMillis = 2000;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
