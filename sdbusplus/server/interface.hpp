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
struct interface final
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
    interface(sdbusplus::bus::bus& bus, const char* path, const char* interf,
              const sdbusplus::vtable::vtable_t* vtable, void* context) :
        _bus(bus.get(), bus.getInterface()),
        _path(path), _interf(interf), _slot(nullptr), _intf(bus.getInterface()),
        _interface_added(false)
    {
        sd_bus_slot* slot = nullptr;
        int r = _intf->sd_bus_add_object_vtable(
            _bus.get(), &slot, _path.c_str(), _interf.c_str(), vtable, context);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_add_object_vtable");
        }

        _slot = decltype(_slot){slot};
    }

    ~interface()
    {
        emit_removed();
    }

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
    void property_changed(const char* property)
    {
        std::vector<std::string> values{property};
        sdbusplus::bus::details::Strv p(values);

        // Note: Converting to use _strv version, could also mock two pointer
        // use-case explicitly.
        _intf->sd_bus_emit_properties_changed_strv(
            _bus.get(), _path.c_str(), _interf.c_str(), static_cast<char**>(p));
    }

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

    bus::bus& bus()
    {
        return _bus;
    }
    const std::string& path()
    {
        return _path;
    }

  private:
    bus::bus _bus;
    std::string _path;
    std::string _interf;
    slot::slot _slot;
    SdBusInterface* _intf;
    bool _interface_added;
};

} // namespace interface

using interface_t = interface::interface;

} // namespace server
} // namespace sdbusplus
