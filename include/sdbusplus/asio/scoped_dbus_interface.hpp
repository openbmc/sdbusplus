#pragma once

#include <memory>
#include <string>

namespace sdbusplus::asio
{

class object_server;
class dbus_interface;

class scoped_dbus_interface
{
  public:
    scoped_dbus_interface() = default;
    scoped_dbus_interface(
        sdbusplus::asio::object_server& objServer,
        const std::weak_ptr<sdbusplus::asio::dbus_interface>& weakInterface);

    scoped_dbus_interface(const scoped_dbus_interface&) = delete;
    scoped_dbus_interface(scoped_dbus_interface&&) = default;

    ~scoped_dbus_interface();

    scoped_dbus_interface& operator=(const scoped_dbus_interface&) = delete;
    scoped_dbus_interface& operator=(scoped_dbus_interface&&) = default;

  private:
    sdbusplus::asio::object_server* objServer;
    std::weak_ptr<sdbusplus::asio::dbus_interface> weakInterface;
};

} // namespace sdbusplus::asio
