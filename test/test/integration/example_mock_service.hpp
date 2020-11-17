#pragma once

#include <net/example/Software/Updater/mock_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>

#include <memory>
#include <string>

#include "gmock/gmock.h"

using sdbusplus::test::integration::MockObject;
using sdbusplus::test::integration::MockService;
using sdbusplus::test::integration::PrivateBus;

using ::testing::NiceMock;

using sdbusplus::net::example::Software::server::MockUpdater;

using MockSoftwareUpdaterObjectBase = sdbusplus::server::object_t<MockUpdater>;

class SoftwareUpdaterObject : public MockObject
{
  public:
    SoftwareUpdaterObject(const std::string& path, uint64_t initialVersion) :
        MockObject(path), initialVersion(initialVersion){};

    void start(sdbusplus::bus::bus& bus) override
    {
        MockObject::start(bus);
        mockBase = std::make_shared<NiceMock<MockSoftwareUpdaterObjectBase>>(
            bus, getPath().c_str());
        mockBase->setPropertyByName("Version", initialVersion);
    };

    std::shared_ptr<MockSoftwareUpdaterObjectBase> getMockBase()
    {
        return mockBase;
    };

  private:
    uint64_t initialVersion;
    std::shared_ptr<MockSoftwareUpdaterObjectBase> mockBase;
};

class SoftwareUpdaterService : public MockService
{
  public:
    SoftwareUpdaterService(std::string name,
                           std::shared_ptr<PrivateBus> privateBus,
                           sdbusplus::SdBusDuration microsecondsToRun) :
        MockService(name, privateBus, microsecondsToRun){};

    void addSoftwareUpdaterObject(const std::string& objectPath,
                                  uint64_t version)
    {
        addObject(std::make_shared<SoftwareUpdaterObject>(objectPath, version));
    };

    SoftwareUpdaterObject& getObject(std::string path)
    {
        return *(
            std::dynamic_pointer_cast<SoftwareUpdaterObject>(objectRepo[path]));
    };
};
