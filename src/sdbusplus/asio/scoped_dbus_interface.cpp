#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/scoped_dbus_interface.hpp>

namespace sdbusplus::asio
{

scoped_dbus_interface::scoped_dbus_interface(
    sdbusplus::asio::object_server& objServer,
    const std::weak_ptr<sdbusplus::asio::dbus_interface>& weakInterface) :
    objServer(&objServer),
    weakInterface(weakInterface)
{}

scoped_dbus_interface::~scoped_dbus_interface()
{
    if (auto interface = weakInterface.lock())
    {
        objServer->remove_interface(interface);
    }
}

} // namespace sdbusplus::asio
