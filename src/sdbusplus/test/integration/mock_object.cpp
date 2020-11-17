#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>

#include <memory>
#include <string>

namespace sdbusplus
{

namespace test
{

namespace integration
{

MockObject::MockObject(std::string objPath) : path(objPath)
{}

std::string MockObject::getPath()
{
    return this->path;
}

void MockObject::start(sdbusplus::bus::bus& bus)
{
    manager = std::make_shared<sdbusplus::server::manager_t>(bus, path.c_str());
}

} // namespace integration
} // namespace test
} // namespace sdbusplus
