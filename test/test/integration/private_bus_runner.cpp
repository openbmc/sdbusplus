
#include <sdbusplus/test/integration/private_bus.hpp>

#include <cassert>
#include <string>

using sdbusplus::test::integration::PrivateBus;

int main()
{
    PrivateBus mockBus;
    std::string serviceName = "sdbusplus.test.integration.FakeService";
    mockBus.registerService(serviceName);
    auto bus = mockBus.getBus(serviceName);
    auto names = bus->list_names_acquired();
    assert(std::find(names.begin(), names.end(), serviceName) != names.end());
    return 0;
}
