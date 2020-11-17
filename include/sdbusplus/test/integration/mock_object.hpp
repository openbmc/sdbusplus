#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>

#include <memory>
#include <string>

namespace sdbusplus
{

namespace test
{

namespace integration
{

/** This class includes common functionalities that are shared by all
 * mock objects.
 */
class MockObject
{
  public:
    /** Constructs with the path which this object on D-Bus will be placed on.
     *
     * This mock object will not be plcaed on D-Bus in the constructor.
     */
    MockObject(std::string objPath);

    virtual ~MockObject() = default;

    std::string getPath();

    /** Registers the path for this object in dbus object manager
     *
     * When services have their bus connections, they can call the start
     * function. The classes that inherit this base class should extend this
     * method and place their mock object with the implemented mock interfaces
     * on the bus.
     * @see examples in the unit test for MockService and other examples in
     * test/integration directory of pid-control and host-ipmid.
     */
    virtual void start(sdbusplus::bus::bus& bus);

  private:
    std::string path;

    /**  An sd-bus object manager for the object represented by this class.
     */
    std::shared_ptr<sdbusplus::server::manager_t> manager;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
