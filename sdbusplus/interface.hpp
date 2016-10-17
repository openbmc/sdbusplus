#pragma once

#include <systemd/sd-bus.h>
#include <sdbusplus/slot.hpp>
#include <sdbusplus/vtable.hpp>
#include <sdbusplus/bus.hpp>

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
 *  inheritence, which might be a natural option.  Instead, a dbus interface
 *  implementation should 'has-a' server::interface with a name sufficiently
 *  unique to prevent name collisions in multiple-inheritence situations.
 */
struct interface final
{
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
    interface() = delete;
    interface(const interface&) = delete;
    interface& operator=(const interface&) = delete;
    interface(interface&&) = default;
    interface& operator=(interface&&) = default;
    ~interface() = default;

    /** @brief Register the (path, interface, vtable) as a dbus object.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] path - The path to register as.
     *  @param[in] interf - The interface to register as.
     *  @param[in] vtable - The vtable to register.
     *  @param[in] context - User-defined context, which is often 'this' from
     *                       the interface implementation class.
     */
    interface(sdbusplus::bus::bus& bus,
              const char* path,
              const char* interf,
              const sdbusplus::vtable::vtable_t* vtable,
              void* context) : _slot(nullptr)
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_object_vtable(bus.get(), &slot, path, interf,
                                 vtable, context);

        _slot = decltype(_slot){slot};
    }

    private:
        slot::slot _slot;
};

} // namespace interface
} // namespace server
} // namespace sdbusplus
