#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/mock_object.hpp>
#include <sdbusplus/test/integration/private_bus.hpp>

#include <atomic>
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

/** This class includes common functionalities that are shared by all
 * mock services.
 *
 * It has a worker thread, which is responsible for registering the service
 * on D-Bus and running the sdbus event loop during the test.
 */
class MockService
{
  public:
    /** Constructs the service with specfied service name, a pointer to this
     * test’s exclusive bus ( @see PrivateBus ), and the duration that this
     * service is active.
     *
     * The service does not start in the constructor. @see start()
     */
    MockService(std::string serviceName, std::shared_ptr<PrivateBus> privateBus,
                SdBusDuration microsecondsToRun);

    virtual ~MockService();

    /** Starts the service specfied in the constructor.
     *
     * The worker thread gets started in this method.
     */
    void start();

    /** The worker thread automatically finishes after the test duration or
     * upon object destruction. If there is a need to stop the service earlier,
     * the stop method should be called explicitly.
     */
    void stop();

    bool isStarted();

    /** Holds pointers to all objects started by this service.
     */
    std::unordered_map<std::string, std::shared_ptr<MockObject>> objectRepo;

  protected:
    /** Add a new object to this service.
     *
     * It is preferred to add all needed objects before the service starts,
     * because when the service starts, it operates on a separate thread.
     * If an object is added after the service starts, it will be added to the
     * bus during the main event loop execution.
     * @see afterStartPendingObjects
     * @see proceed
     */
    void addObject(std::shared_ptr<MockObject> obj);

    bool hasObjects();

    std::string mainObjectPath;
    std::atomic<bool> started;
    /** This is the method executed by the worker that is started by calling
     * the start method.
     *
     * This thread first registers the service, then gets the bus connection,
     * next, starts the objects, and then goes to the sdbus event loop for the
     * test duration.
     */
    virtual void run();
    /** This method is called in the event loop, each time that the threads
     * wakes up.
     *
     * The base implementation starts the new objects that have been added
     * since the last wakeup call.
     * Services can override this method to add customized actions within the
     * event loop. For example, @see the temperature sensor can increase its
     * value each second.
     */
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
