#include <exception>
#include <map>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/sdbuspp_support/server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <tuple>

#include <${interface.headerFile("server")}>
% for m in interface.methods + interface.properties + interface.signals:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp-includes') }
% endfor
namespace sdbusplus::server::${interface.cppNamespace()}
{

    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for p in interface.properties:
${ p.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
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
${ m.cpp_prototype(loader, interface=interface, ptype='vtable') }
    % endfor
    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='vtable') }
    % endfor
    % for p in interface.properties:
    vtable::property("${p.name}",
                     details::${interface.classname}::_property_${p.name}
                        .data(),
                     _callback_get_${p.name},
        % if 'const' not in p.flags and 'readonly' not in p.flags:
                     _callback_set_${p.name},
        % endif
        % if not p.cpp_flags:
                     vtable::property_::emits_change),
        % else:
                     ${p.cpp_flags}),
        % endif
    % endfor
    vtable::end()
};

} // namespace sdbusplus::server::${interface.cppNamespace()}
