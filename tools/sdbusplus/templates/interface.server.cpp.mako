#include <algorithm>
#include <map>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <string>
#include <tuple>
#include <variant>

#include <${"/".join(interface.name.split('.') + [ 'server.hpp' ])}>
% for m in interface.methods + interface.properties + interface.signals:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp-includes') }
% endfor
<%
    namespaces = interface.name.split('.')
    classname = namespaces.pop()

    def interface_instance():
        return "_".join(interface.name.split('.') + ['interface'])
%>
namespace sdbusplus
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
namespace server
{

${classname}::${classname}(bus::bus& bus, const char* path)
        : _${interface_instance()}(
                bus, path, interface, _vtable, this), _intf(bus.getInterface())
{
}

    % if interface.properties:
${classname}::${classname}(bus::bus& bus, const char* path,
                           const std::map<std::string, PropertiesVariant>& vals,
                           bool skipSignal)
        : ${classname}(bus, path)
{
    for (const auto& v : vals)
    {
        setPropertyByName(v.first, v.second, skipSignal);
    }
}

    % endif
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
void ${classname}::setPropertyByName(const std::string& _name,
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

auto ${classname}::getPropertyByName(const std::string& _name) ->
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
    % for e in interface.enums:

namespace
{
/** String to enum mapping for ${classname}::${e.name} */
static const std::tuple<const char*, ${classname}::${e.name}> \
mapping${classname}${e.name}[] =
        {
        % for v in e.values:
            std::make_tuple( "${interface.name}.${e.name}.${v.name}", \
                ${classname}::${e.name}::${v.name} ),
        % endfor
        };

} // anonymous namespace

auto ${classname}::convert${e.name}FromString(const std::string& s) ->
        ${e.name}
{
    auto i = std::find_if(
            std::begin(mapping${classname}${e.name}),
            std::end(mapping${classname}${e.name}),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mapping${classname}${e.name}) == i)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return std::get<1>(*i);
    }
}

std::string ${classname}::convert${e.name}ToString(${classname}::${e.name} v)
{
    auto i = std::find_if(
            std::begin(mapping${classname}${e.name}),
            std::end(mapping${classname}${e.name}),
            [v](auto& e){ return v == std::get<1>(e); });
    if (i == std::end(mapping${classname}${e.name}))
    {
        throw std::invalid_argument(std::to_string(static_cast<int>(v)));
    }
    return std::get<0>(*i);
}
    % endfor

const vtable::vtable_t ${classname}::_vtable[] = {
    vtable::start(),
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='vtable') }
    % endfor
    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='vtable') }
    % endfor
    % for p in interface.properties:
    vtable::property("${p.name}",
                     details::${classname}::_property_${p.name}
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

} // namespace server
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
