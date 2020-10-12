#include <sdbusplus/server/interface.hpp>

namespace sdbusplus
{

namespace server
{

namespace interface
{

interface::interface(sdbusplus::bus::bus& bus, const char* path,
                     const char* interf,
                     const sdbusplus::vtable::vtable_t* vtable, void* context) :
    _bus(bus.get(), bus.getInterface()),
    _path(path), _interf(interf), _slot(nullptr), _intf(bus.getInterface()),
    _interface_added(false)
{
    sd_bus_slot* slot = nullptr;
    int r = _intf->sd_bus_add_object_vtable(_bus.get(), &slot, _path.c_str(),
                                            _interf.c_str(), vtable, context);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_add_object_vtable");
    }

    _slot = decltype(_slot){slot};
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
    _intf->sd_bus_emit_properties_changed_strv(_bus.get(), _path.c_str(),
                                               _interf.c_str(), values.data());
}

} // namespace interface
} // namespace server
} // namespace sdbusplus
