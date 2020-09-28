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
        const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface);

    scoped_dbus_interface(const scoped_dbus_interface&) = delete;
    scoped_dbus_interface(scoped_dbus_interface&&) = default;

    ~scoped_dbus_interface();

    scoped_dbus_interface& operator=(const scoped_dbus_interface&) = delete;
    scoped_dbus_interface& operator=(scoped_dbus_interface&&) = default;

    sdbusplus::asio::dbus_interface* operator->()
    {
        return interface.get();
    }

    const sdbusplus::asio::dbus_interface* operator->() const
    {
        return interface.get();
    }

    sdbusplus::asio::dbus_interface* operator*()
    {
        return interface.get();
    }

    const sdbusplus::asio::dbus_interface* operator*() const
    {
        return interface.get();
    }

  private:
    sdbusplus::asio::object_server* objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> interface;
};

} // namespace sdbusplus::asio
