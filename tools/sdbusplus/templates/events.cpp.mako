#include <${events.headerFile("event")}>
#include <nlohmann/json.hpp>

%if events.errors:

namespace sdbusplus::error::${events.cppNamespacedClass()}
{
% for e in events.errors:

${events.render(loader, "event.cpp.mako", events=events, event=e)}\
% endfor

/* Load error map into sd_bus for errno translation. */
static sd_bus_error_map errors[] = {
% for e in events.errors:
    SD_BUS_ERROR_MAP(${e.CamelCase}::errName, ${e.CamelCase}::errErrno),
% endfor
    SD_BUS_ERROR_MAP_END
};
[[gnu::constructor]] static void loadErrors()
{
    sd_bus_error_add_map(errors);
}

} // namespace sdbusplus::error::${events.cppNamespacedClass()}

%endif
%if events.events:

namespace sdbusplus::event::${events.cppNamespacedClass()}
{
% for e in events.events:

${events.render(loader, "event.cpp.mako", events=events, event=e)}\
% endfor

} // namespace sdbusplus::event::${events.cppNamespacedClass()}
%endif
