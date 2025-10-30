#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/slot.hpp>
#include <sdbusplus/vtable.hpp>

#include <string>

namespace sdbusplus
{

namespace server
{

namespace interface
{

/** @class interface
 *  @brief Provide a C++ holder for dbus interface instances.
 *
 *  Wraps sd_bus_add_object_vtable so that the interface is registered
 *  on construction and deregistered on destruction.
 *
 *  @note This class is 'final' because it is expected to be used by an
 *  implementation of a class representing a dbus interface, which will be
 *  composed through multiple-inheritence to create a single dbus 'object'.
 *  Marking it 'final' prevents users from using an 'is-a' relationship via
 *  inheritance, which might be a natural option.  Instead, a dbus interface
 *  implementation should 'has-a' server::interface with a name sufficiently
 *  unique to prevent name collisions in multiple-inheritence situations.
 */
struct interface final : private sdbusplus::bus::details::bus_friend
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *         - Move operations.
     *     Allowed:
     *         - Destructor.
     */
    interface() = delete;
    interface(const interface&) = delete;
    interface& operator=(const interface&) = delete;
    interface(interface&&) = delete;
    interface& operator=(interface&&) = delete;

    /** @brief Register the (path, interface, vtable) as a dbus object.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] path - The path to register as.
     *  @param[in] interf - The interface to register as.
     *  @param[in] vtable - The vtable to register.
     *  @param[in] context - User-defined context, which is often 'this' from
     *                       the interface implementation class.
     */
    interface(sdbusplus::bus_t& bus, const char* path, const char* interf,
              const sdbusplus::vtable_t* vtable, void* context);

    interface(sdbusplus::bus_t& bus,
              const sdbusplus::message::object_path& path, const char* interf,
              const sdbusplus::vtable_t* vtable, void* context);

    ~interface();

    /** @brief Create a new signal message.
     *
     *  @param[in] member - The signal name to create.
     */
    auto new_signal(const char* member)
    {
        return _bus.new_signal(_path.c_str(), _interf.c_str(), member);
    }

    /** @brief Broadcast a property changed signal.
     *
     *  @param[in] property - The property which changed.
     */
    void property_changed(const char* property);

    /** @brief Emit the interface is added on D-Bus */
    void emit_added()
    {
        if (!_interface_added)
        {
            _bus.emit_interfaces_added(_path.c_str(), {_interf});
            _interface_added = true;
        }
    }

    /** @brief Emit the interface is removed on D-Bus */
    void emit_removed()
    {
        if (_interface_added)
        {
            _bus.emit_interfaces_removed(_path.c_str(), {_interf});
            _interface_added = false;
        }
    }

    bus_t& bus()
    {
        return _bus;
    }
    const std::string& path()
    {
        return _path;
    }

  private:
    bus_t _bus;
    std::string _path;
    std::string _interf;
    bool _interface_added;
    slot_t _slot;
};

} // namespace interface

using interface_t = interface::interface;

} // namespace server
} // namespace sdbusplus
