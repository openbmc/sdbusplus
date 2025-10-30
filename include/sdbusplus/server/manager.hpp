#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/slot.hpp>

namespace sdbusplus
{

namespace server
{

namespace manager
{

/** @class manager
 *  @brief Provide a C++ holder for a sd-bus object manager.
 *
 *  Wraps sd_bus_add_object_manager so that an sd-bus owned object manager
 *  can be installed at a path and automatically removed when destructed.
 */
struct manager : private sdbusplus::bus::details::bus_friend
{
  private:
    slot_t _slot;

    static slot_t makeManager(SdBusInterface* intf, sd_bus* bus,
                              const char* path)
    {
        sd_bus_slot* slot = nullptr;
        intf->sd_bus_add_object_manager(bus, &slot, path);
        return slot_t{slot, intf};
    }

    static slot_t makeManager(SdBusInterface* intf, sd_bus* bus,
                              const sdbusplus::message::object_path& path)
    {
        return makeManager(intf, bus, path.str.c_str());
    }

  public:
    /** @brief Register an object manager at a path.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] path - The path to register.
     */
    manager(sdbusplus::bus_t& bus, const char* path) :
        _slot(makeManager(bus.getInterface(), get_busp(bus), path))
    {}

    manager(sdbusplus::bus_t& bus,
            const sdbusplus::message::object_path& path) :
        _slot(makeManager(bus.getInterface(), get_busp(bus), path))
    {}
};

} // namespace manager

using manager_t = manager::manager;

} // namespace server
} // namespace sdbusplus
