#include "example_mock_service.hpp"

#include <net/example/Software/Updater/mock_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <sdbusplus/test/integration/test_timer.hpp>

#include <chrono>
#include <memory>
#include <thread>

#include "gmock/gmock.h"
#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;

static const auto lettingServiceStart = 60ms;
static const auto secondsToRunTest = lettingServiceStart * 5;
static const auto uSecondsToRunTest =
    std::chrono::microseconds(secondsToRunTest);

using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockService;
using sdbusplus::test::integration::PrivateBus;
using sdbusplus::test::integration::Timer;

class MockServiceTest : public ::testing::Test
{
  public:
    MockServiceTest()
    {
        testServiceName = "sdbusplus.test.integration.MainTestService";
        mockBus = std::make_shared<PrivateBus>();
        mockBus->registerService(testServiceName);
        bus = mockBus->getBus(testServiceName);
    };

  protected:
    void expectServiceNameOnBus(const std::string& name)
    {
        auto names = bus->list_names_acquired();
        EXPECT_NE(std::find(names.begin(), names.end(), name), names.end());
    };

    void expectInterfaceIntrospect(const std::string& serviceName,
                                   const std::string& objectPath)
    {
        auto method = bus->new_method_call(
            serviceName.c_str(), objectPath.c_str(),
            "org.freedesktop.DBus.Introspectable", "Introspect");
        auto reply = bus->call(method);
        const char* introspect_xml;
        reply.read(introspect_xml);
        EXPECT_NE(std::string(introspect_xml).find(expectedStrIntrospect),
                  std::string::npos);
    };

    std::shared_ptr<PrivateBus> mockBus;
    std::string testServiceName;
    std::shared_ptr<sdbusplus::bus::bus> bus;

    static constexpr std::string_view expectedStrIntrospect = R"(
 <interface name="net.example.Software.Updater">
  <method name="Reboot">
   <arg type="t" direction="in"/>
   <arg type="t" direction="out"/>
   <annotation name="org.freedesktop.systemd1.Privileged" value="true"/>
  </method>
  <property name="Version" type="t" access="readwrite">
   <annotation name="org.freedesktop.systemd1.Privileged" value="true"/>
  </property>
 </interface>)";
};

TEST_F(MockServiceTest, AddObjectAfterServiceStart)
{
    std::string serviceName = "sdbusplus.test.integration.SoftwareUpdater";
    std::string objectPath = "/a/fake/object/path/for/SoftwareUpdater";
    uint64_t initivalVersion = 1;

    SoftwareUpdaterService service(serviceName, mockBus, uSecondsToRunTest);
    service.start();
    std::this_thread::sleep_for(lettingServiceStart);

    EXPECT_TRUE(service.isStarted());
    EXPECT_EQ(service.objectRepo.size(), 0);

    service.addSoftwareUpdaterObject(objectPath, initivalVersion);

    expectServiceNameOnBus(serviceName);
    expectInterfaceIntrospect(serviceName, objectPath);
    EXPECT_EQ(service.objectRepo.size(), 1);
}

TEST_F(MockServiceTest, AddObjectBeforeServiceStart)
{
    std::string serviceName = "sdbusplus.test.integration.SoftwareUpdater";
    std::string objectPath = "/a/fake/object/path/for/SoftwareUpdater";
    uint64_t initivalVersion = 1;

    SoftwareUpdaterService service(serviceName, mockBus, uSecondsToRunTest);
    service.addSoftwareUpdaterObject(objectPath, initivalVersion);

    EXPECT_EQ(service.objectRepo.size(), 1);

    service.start();
    std::this_thread::sleep_for(lettingServiceStart);

    EXPECT_TRUE(service.isStarted());
    expectInterfaceIntrospect(serviceName, objectPath);
    expectServiceNameOnBus(serviceName);
}

TEST_F(MockServiceTest, ObjectPorpsGetterSetter)
{
    std::string serviceName = "sdbusplus.test.integration.SoftwareUpdater";
    std::string objectPath = "/a/fake/object/path/for/SoftwareUpdater";
    uint64_t initivalVersion = 1;

    SoftwareUpdaterService service(serviceName, mockBus, uSecondsToRunTest);
    service.addSoftwareUpdaterObject(objectPath, initivalVersion);

    EXPECT_EQ(service.objectRepo.size(), 1);

    service.start();
    std::this_thread::sleep_for(lettingServiceStart);

    EXPECT_TRUE(service.isStarted());

    auto mockObjPtr = service.getObject(objectPath).getMockBase();

    EXPECT_EQ(mockObjPtr->version(), initivalVersion);

    uint64_t secondVersion = initivalVersion + 1;
    mockObjPtr->version(secondVersion);

    EXPECT_EQ(mockObjPtr->version(), secondVersion);
}

TEST_F(MockServiceTest, ObjectMethodMock)
{
    std::string serviceName = "sdbusplus.test.integration.SoftwareUpdater";
    std::string objectPath = "/a/fake/object/path/for/SoftwareUpdater";
    uint64_t initivalVersion = 1;
    uint64_t paramToMethodCall = 10;
    SoftwareUpdaterService service(serviceName, mockBus, uSecondsToRunTest);
    service.addSoftwareUpdaterObject(objectPath, initivalVersion);

    EXPECT_EQ(service.objectRepo.size(), 1);

    service.start();
    std::this_thread::sleep_for(lettingServiceStart);

    EXPECT_EQ(service.isStarted(), true);
    expectServiceNameOnBus(serviceName);
    expectInterfaceIntrospect(serviceName, objectPath);

    auto mockObjPtr = service.getObject(objectPath).getMockBase();

    EXPECT_CALL(*mockObjPtr, reboot(paramToMethodCall))
        .WillOnce([](uint64_t input) { return input + 1; });

    auto method = bus->new_method_call(serviceName.c_str(), objectPath.c_str(),
                                       MockUpdater::interface, "Reboot");
    method.append(paramToMethodCall);
    auto reply = bus->call(method);
    uint64_t response;
    reply.read(response);

    EXPECT_EQ(response, paramToMethodCall + 1);
}
