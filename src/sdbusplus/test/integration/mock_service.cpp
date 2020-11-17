#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/mock_service.hpp>
#include <sdbusplus/test/integration/test_timer.hpp>

#include <chrono> // std::chrono::seconds
#include <memory>
#include <string>
#include <thread>

namespace sdbusplus
{

namespace test
{

namespace integration
{

MockService::MockService(std::shared_ptr<sdbusplus::bus::bus> inBus,
                         std::string serviceName) :
    started(false),
    name(serviceName), bus(*inBus)
{
    registerService();
}

sdbusplus::bus::bus& MockService::getBus()
{
    return bus;
}

bool MockService::hasObjects()
{
    return !objectRepo.empty();
}

bool MockService::isStarted()
{
    return started;
}

void MockService::addObject(std::shared_ptr<MockObject> obj)
{
    if (!hasObjects())
    {
        mainObjectPath = obj->getPath();
    }
    objectRepo.emplace(obj->getPath(), obj);
}

void MockService::registerService()
{
    bus.request_name(name.c_str());
}

MockServiceActive::MockServiceActive(std::shared_ptr<sdbusplus::bus::bus> inBus,
                                     std::string serviceName,
                                     SdBusDuration microsecondsToRun,
                                     SdBusDuration microsecondsBetweenSteps) :
    MockService(inBus, serviceName),
    activeDuration(microsecondsToRun),
    timeBetweenSteps(microsecondsBetweenSteps),
    worker(&MockServiceActive::run, this)
{}

MockServiceActive::~MockServiceActive()
{
    if (worker.joinable())
    {
        worker.join();
    }
}

void MockServiceActive::run()
{
    if (isStarted())
    {
        Timer test;
        auto duration = test.duration();
        while (duration < activeDuration)
        {
            proceed();
            duration = test.duration();
        }
    }
}

void MockServiceActive::proceed()
{
    std::this_thread::sleep_for(timeBetweenSteps);
}

} // namespace integration
} // namespace test
} // namespace sdbusplus
