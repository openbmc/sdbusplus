#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/private_bus.hpp>

#include <memory>
#include <mutex>
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
    MockService(std::string serviceName, std::shared_ptr<PrivateBus> privateBus,
                SdBusDuration microsecondsToRun);

    virtual ~MockService();

    void start();

    void stop();

    bool isStarted();

    std::unordered_map<std::string, std::shared_ptr<MockObject>> objectRepo;

  protected:
    void addObject(std::shared_ptr<MockObject> obj);

    bool hasObjects();

    std::string mainObjectPath;
    bool started;
    virtual void run();
    virtual void proceed();

  private:
    void registerService();
    void startObjects();

    std::string name;
    std::shared_ptr<PrivateBus> mockBus;
    SdBusDuration activeDuration;
    std::shared_ptr<std::thread> worker;
    std::shared_ptr<sdbusplus::bus::bus> bus;

    std::vector<std::string> afterStartPendingObjects;
    std::mutex afterStartPendingObjectsMutex;

    bool shouldWork;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
