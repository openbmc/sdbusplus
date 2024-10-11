#include <sdbusplus/bus.hpp>

namespace sdbusplus::bus
{

void bus::emit_interfaces_added(const char* path,
                                const std::vector<std::string>& ifaces)
{
    details::Strv s{ifaces};
    _intf->sd_bus_emit_interfaces_added_strv(_bus.get(), path,
                                             static_cast<char**>(s));
}

void bus::emit_interfaces_removed(const char* path,
                                  const std::vector<std::string>& ifaces)
{
    details::Strv s{ifaces};
    _intf->sd_bus_emit_interfaces_removed_strv(_bus.get(), path,
                                               static_cast<char**>(s));
}

/* Create a new default connection: system bus if root, session bus if user. */
bus new_default()
{
    sd_bus* b = nullptr;
    sd_bus_default(&b);
    return bus(b, std::false_type());
}

/* Create a new default connection to the session bus. */
bus new_default_user()
{
    sd_bus* b = nullptr;
    sd_bus_default_user(&b);
    return bus(b, std::false_type());
}

/* Create a new default connection to the system bus. */
bus new_default_system()
{
    sd_bus* b = nullptr;
    sd_bus_default_system(&b);
    return bus(b, std::false_type());
}

/* Create a new connection: system bus if root, session bus if user. */
bus new_bus()
{
    sd_bus* b = nullptr;
    sd_bus_open(&b);
    bus bus(b, std::false_type());
    bus.set_should_close(true);
    return bus;
}

/* Create a new connection to the session bus. */
bus new_user()
{
    sd_bus* b = nullptr;
    sd_bus_open_user(&b);
    bus bus(b, std::false_type());
    bus.set_should_close(true);
    return bus;
}

/* Create a new connection to the system bus. */
bus new_system()
{
    sd_bus* b = nullptr;
    sd_bus_open_system(&b);
    bus bus(b, std::false_type());
    bus.set_should_close(true);
    return bus;
}

bus::bus(busp_t b, sdbusplus::SdBusInterface* intf) :
    _intf(intf), _bus(_intf->sd_bus_ref(b), details::BusDeleter(intf))
{
    // Emitting object added causes a message to get the properties
    // which can trigger a 'transaction' in the server bindings.  If
    // the bus isn't up far enough, this causes an assert deep in
    // sd-bus code.  Get the 'unique_name' to ensure the bus is up far
    // enough to avoid the assert.
    if (b != nullptr)
    {
        get_unique_name();
    }
}

bus::bus(busp_t b) :
    _intf(&sdbus_impl),
    _bus(_intf->sd_bus_ref(b), details::BusDeleter(&sdbus_impl))
{
    // Emitting object added causes a message to get the properties
    // which can trigger a 'transaction' in the server bindings.  If
    // the bus isn't up far enough, this causes an assert deep in
    // sd-bus code.  Get the 'unique_name' to ensure the bus is up far
    // enough to avoid the assert.
    if (b != nullptr)
    {
        get_unique_name();
    }
}

bus::bus(busp_t b, std::false_type) :
    _intf(&sdbus_impl), _bus(b, details::BusDeleter(&sdbus_impl))
{
    // Emitting object added causes a message to get the properties
    // which can trigger a 'transaction' in the server bindings.  If
    // the bus isn't up far enough, this causes an assert deep in
    // sd-bus code.  Get the 'unique_name' to ensure the bus is up far
    // enough to avoid the assert.
    if (b != nullptr)
    {
        get_unique_name();
    }
}

} // namespace sdbusplus::bus
