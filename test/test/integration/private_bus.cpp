#include <sdbusplus/test/integration/private_bus.hpp>

#include <cstdlib>
#include <exception>
#include <thread>

#include <gtest/gtest.h>

using sdbusplus::test::integration::PrivateBus;

class PrivateBusTest : public ::testing::Test
{
  public:
    void runPrivateBusMain()
    {
        auto rc = std::system("test/test/integration/private_bus_runner");
        EXPECT_EQ(rc, 0);
    };
};

TEST_F(PrivateBusTest, RegisterServiceOnBus)
{
    PrivateBus mockBus;
    std::string serviceName = "sdbusplus.test.integration.FakeService";
    mockBus.registerService(serviceName);
    auto bus = mockBus.getBus(serviceName);
    auto names = bus->list_names_acquired();
    EXPECT_NE(std::find(names.begin(), names.end(), serviceName), names.end());
    mockBus.unregisterService(serviceName);
    bus = mockBus.getBus(serviceName);
    EXPECT_EQ(bus, nullptr);
}

TEST_F(PrivateBusTest, ErrorOnSameNameFromSameProcess)
{
    PrivateBus mockBus1;
    PrivateBus mockBus2;
    std::string serviceName = "sdbusplus.test.integration.FakeService";
    mockBus1.registerService(serviceName);
    EXPECT_THROW(mockBus2.registerService(serviceName),
                 sdbusplus::exception::SdBusError);
}

TEST_F(PrivateBusTest, RegisterSameNameFromConcurrentProcesses)
{
    std::thread t1(std::bind(&PrivateBusTest::runPrivateBusMain, this));
    std::thread t2(std::bind(&PrivateBusTest::runPrivateBusMain, this));
    t1.join();
    t2.join();
}
