#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>

using namespace std::literals::chrono_literals;

namespace sdbusplus
{

namespace test
{

namespace integration
{

class MockService
{
  public:
    MockService(std::shared_ptr<sdbusplus::bus::bus> inBus,
                std::string serviceName);

    virtual ~MockService() = default;

    sdbusplus::bus::bus& getBus();

    bool isStarted();

    std::unordered_map<std::string, std::shared_ptr<MockObject>> objectRepo;

  protected:
    void addObject(std::shared_ptr<MockObject> obj);

    bool hasObjects();

    std::string mainObjectPath;
    bool started;

  private:
    void registerService();

    std::string name;
    sdbusplus::bus::bus& bus;
};

class MockServiceActive : public MockService
{
  public:
    MockServiceActive(std::shared_ptr<sdbusplus::bus::bus> inBus,
                      std::string serviceName, SdBusDuration microsecondsToRun,
                      SdBusDuration timeBetweenSteps = 1s);

    virtual ~MockServiceActive();

  protected:
    virtual void run();
    virtual void proceed();

  private:
    SdBusDuration activeDuration;
    SdBusDuration timeBetweenSteps;
    std::thread worker;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
