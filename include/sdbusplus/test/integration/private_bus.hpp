#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/dbusd_manager.hpp>

#include <memory>
#include <unordered_map>

namespace sdbusplus
{

namespace test
{

namespace integration
{

/** This class hides the details of bus connection and daemon management.
 *
 * It allows to have an exclusive bus per test that enables parallel tests
 * with the same service names.
 */
class PrivateBus
{
  public:
    PrivateBus(const PrivateBus&) = delete;
    PrivateBus& operator=(const PrivateBus&) = delete;
    PrivateBus(PrivateBus&&) = delete;
    PrivateBus& operator=(PrivateBus&&) = delete;

    /** Constructs the private bus.
     *
     * It starts the dbus daemon in constructor.
     */
    PrivateBus();

    /** Registers a service with the specified name on D-bus.
     *
     * Since the connection to a bus is associated with the calling thread,
     * each service should do the registration from its own thread
     * (and not the main thread).
     * @param name that is used as the D-Bus well-known name.
     */
    void registerService(const std::string& name);

    /** Removes the bus connection for the service specified by name.
     */
    void unregisterService(const std::string& name);

    /** Get the bus connection for the service specified by name.
     */
    std::shared_ptr<sdbusplus::bus::bus> getBus(const std::string& name);

  private:
    DBusDaemon daemon;

    /** An internal repository for keeping bus connections for each service.
     */
    std::unordered_map<std::string, std::shared_ptr<sdbusplus::bus::bus>>
        busMap;

    /** The default time to wait for the dbus daemon to setup.
     */
    static const int defaultWarmupMillis = 2000;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
