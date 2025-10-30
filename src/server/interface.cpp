#include <sdbusplus/server/interface.hpp>

namespace sdbusplus
{

namespace server
{

namespace interface
{

static slot_t makeObjVtable(SdBusInterface* intf, sd_bus* bus, const char* path,
                            const char* interf,
                            const sdbusplus::vtable_t* vtable, void* context)
{
    sd_bus_slot* slot;
    int r = intf->sd_bus_add_object_vtable(bus, &slot, path, interf, vtable,
                                           context);
    if (r < 0)
    {
        throw exception::SdBusError(-r, "sd_bus_add_object_vtable");
    }
    return slot_t{slot, intf};
}

interface::interface(sdbusplus::bus_t& bus, const char* path,
                     const char* interf, const sdbusplus::vtable_t* vtable,
                     void* context) :
    _bus(get_busp(bus), bus.getInterface()), _path(path), _interf(interf),
    _interface_added(false),
    _slot(makeObjVtable(_bus.getInterface(), get_busp(_bus), _path.c_str(),
                        _interf.c_str(), vtable, context))
{}

interface::interface(sdbusplus::bus_t& bus,
                     const sdbusplus::message::object_path& path,
                     const char* interf, const sdbusplus::vtable_t* vtable,
                     void* context) :
    interface(bus, path.str.c_str(), interf, vtable, context)
{}

interface::~interface()
{
    emit_removed();
}

void interface::property_changed(const char* property)
{
    std::array<const char*, 2> values = {property, nullptr};

    // Note: Converting to use _strv version, could also mock two pointer
    // use-case explicitly.
    _bus.getInterface()->sd_bus_emit_properties_changed_strv(
        get_busp(_bus), _path.c_str(), _interf.c_str(), values.data());
}

} // namespace interface
} // namespace server
} // namespace sdbusplus
