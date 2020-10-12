#include <sdbusplus/bus.hpp>

namespace sdbusplus
{
namespace bus
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

} // namespace bus
} // namespace sdbusplus
