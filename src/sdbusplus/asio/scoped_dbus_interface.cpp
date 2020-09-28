#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/scoped_dbus_interface.hpp>

namespace sdbusplus::asio
{

scoped_dbus_interface::scoped_dbus_interface(
    sdbusplus::asio::object_server& objServer,
    const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) :
    objServer(&objServer),
    interface(interface)
{}

scoped_dbus_interface::~scoped_dbus_interface()
{
    objServer->remove_interface(interface);
}

} // namespace sdbusplus::asio
