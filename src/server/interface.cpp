#include <sdbusplus/server/interface.hpp>

namespace sdbusplus
{

namespace server
{

namespace interface
{

interface::interface(sdbusplus::bus_t& bus, const char* path,
                     const char* interf, const sdbusplus::vtable_t* vtable,
                     void* context) :
    _bus(get_busp(bus), bus.getInterface()),
    _path(path), _interf(interf), _intf(bus.getInterface()),
    _interface_added(false)
{
    sd_bus_slot* slot = nullptr;
    int r = _intf->sd_bus_add_object_vtable(
        get_busp(_bus), &slot, _path.c_str(), _interf.c_str(), vtable, context);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_add_object_vtable");
    }

    _slot = std::move(slot);
}

interface::~interface()
{
    emit_removed();
}

void interface::property_changed(const char* property)
{
    std::array<const char*, 2> values = {property, nullptr};

    // Note: Converting to use _strv version, could also mock two pointer
    // use-case explicitly.
    _intf->sd_bus_emit_properties_changed_strv(get_busp(_bus), _path.c_str(),
                                               _interf.c_str(), values.data());
}

} // namespace interface
} // namespace server
} // namespace sdbusplus
