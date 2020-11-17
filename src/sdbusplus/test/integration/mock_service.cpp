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

MockService::MockService(std::string serviceName,
                         std::shared_ptr<PrivateBus> privateBus,
                         SdBusDuration microsecondsToRun) :
    started(false),
    name(serviceName), mockBus(privateBus), activeDuration(microsecondsToRun)
{}

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
    mockBus->registerService(name);
}

MockService::~MockService()
{
    stop();
    mockBus->unregisterService(name);
}

void MockService::start()
{
    if (!isStarted())
    {
        shouldWork = true;
        worker = std::make_shared<std::thread>(&MockService::run, this);
    }
}

void MockService::stop()
{
    shouldWork = false;
    if (worker->joinable())
    {
        worker->join();
    }
    started = false;
}

void MockService::run()
{
    registerService();
    bus = mockBus->getBus(name);
    startObjects();
    started = true;
    Timer test;
    auto duration = test.duration();
    while (shouldWork && duration < activeDuration)
    {
        bus->process_discard(); // discard any unhandled messages
        bus->wait(activeDuration - duration);
        proceed();
        duration = test.duration();
    }
}

void MockService::startObjects()
{
    for (auto const& [path, object] : objectRepo)
    {
        object->start(*bus);
    }
}

void MockService::proceed()
{}

} // namespace integration
} // namespace test
} // namespace sdbusplus
