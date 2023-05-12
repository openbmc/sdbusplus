#include <exception>
#include <map>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/sdbuspp_support/server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <tuple>

#include <${interface.headerFile("server")}>

namespace sdbusplus::server::${interface.cppNamespace()}
{

    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for p in interface.properties:
${ p.render(loader, "property.server.cpp.mako", property=p, interface=interface) }
    % endfor

    % if interface.properties:
void ${interface.classname}::setPropertyByName(const std::string& _name,
                                     const PropertiesVariant& val,
                                     bool skipSignal)
{
        % for p in interface.properties:
    if (_name == "${p.name}")
    {
        auto& v = std::get<${p.cppTypeParam(interface.name)}>(\
val);
        ${p.camelCase}(v, skipSignal);
        return;
    }
        % endfor
}

auto ${interface.classname}::getPropertyByName(const std::string& _name) ->
        PropertiesVariant
{
    % for p in interface.properties:
    if (_name == "${p.name}")
    {
        return ${p.camelCase}();
    }
    % endfor

    return PropertiesVariant();
}

    % endif


const vtable_t ${interface.classname}::_vtable[] = {
    vtable::start(),

    % for m in interface.methods:
${ m.render(loader, "method.server.vtable.cpp.mako", method=m, interface=interface) }
    % endfor
    % for s in interface.signals:
${ s.render(loader, "signal.server.vtable.cpp.mako", signal=s, interface=interface) }
    % endfor
    % for p in interface.properties:
${ p.render(loader, "property.server.vtable.cpp.mako", property=p, interface=interface) }
    % endfor
    vtable::end()
};

} // namespace sdbusplus::server::${interface.cppNamespace()}
