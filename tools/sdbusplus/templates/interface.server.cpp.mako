#include <algorithm>
#include <map>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/sdbuspp_support/server.hpp>
#include <sdbusplus/server.hpp>
#include <string>
#include <tuple>

#include <${interface.joinedName("/", "server.hpp")}>
% for m in interface.methods + interface.properties + interface.signals:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp-includes') }
% endfor
namespace sdbusplus::${interface.cppNamespace()}
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
    % for e in interface.enums:

namespace
{
/** String to enum mapping for ${interface.classname}::${e.name} */
static const std::tuple<const char*, ${interface.classname}::${e.name}> \
mapping${interface.classname}${e.name}[] =
        {
        % for v in e.values:
            std::make_tuple( "${interface.name}.${e.name}.${v.name}", \
                ${interface.classname}::${e.name}::${v.name} ),
        % endfor
        };

} // anonymous namespace

auto ${interface.classname}::convertStringTo${e.name}(const std::string& s) noexcept ->
        std::optional<${e.name}>
{
    auto i = std::find_if(
            std::begin(mapping${interface.classname}${e.name}),
            std::end(mapping${interface.classname}${e.name}),
            [&s](auto& e){ return 0 == strcmp(s.c_str(), std::get<0>(e)); } );
    if (std::end(mapping${interface.classname}${e.name}) == i)
    {
        return std::nullopt;
    }
    else
    {
        return std::get<1>(*i);
    }
}

auto ${interface.classname}::convert${e.name}FromString(const std::string& s) ->
        ${e.name}
{
    auto r = convertStringTo${e.name}(s);

    if (!r)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    else
    {
        return *r;
    }
}

std::string ${interface.classname}::convert${e.name}ToString(${interface.classname}::${e.name} v)
{
    auto i = std::find_if(
            std::begin(mapping${interface.classname}${e.name}),
            std::end(mapping${interface.classname}${e.name}),
            [v](auto& e){ return v == std::get<1>(e); });
    if (i == std::end(mapping${interface.classname}${e.name}))
    {
        throw std::invalid_argument(std::to_string(static_cast<int>(v)));
    }
    return std::get<0>(*i);
}
    % endfor

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

} // namespace sdbusplus::${interface.cppNamespace()}
