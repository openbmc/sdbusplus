#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <memory>
#include <string>

namespace sdbusplus
{

namespace test
{

namespace integration
{

class MockObject
{
  public:
    MockObject(sdbusplus::bus::bus& bus, std::string objPath);

    virtual ~MockObject() = default;

    std::string getPath();

  private:
    std::string path;
    std::shared_ptr<sdbusplus::server::manager_t> manager;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
