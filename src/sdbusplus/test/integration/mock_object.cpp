#include <sdbusplus/test/integration/mock_object.hpp>

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

MockObject::MockObject(sdbusplus::bus::bus& bus, std::string objPath):
path(objPath),
manager(std::make_shared<sdbusplus::server::manager_t>(bus, path.c_str()))
{
};

std::string MockObject::getPath()
{
    return this->path;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
