#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/dbusd_manager.hpp>

#include <memory>
#include <unordered_map>

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

    void registerService(const std::string& name);
    void unregisterService(const std::string& name);

    std::shared_ptr<sdbusplus::bus::bus> getBus(const std::string& name);

  private:
    DBusDaemon daemon;
    std::unordered_map<std::string, std::shared_ptr<sdbusplus::bus::bus>>
        busMap;

    static const int defaultWarmupMillis = 2000;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
