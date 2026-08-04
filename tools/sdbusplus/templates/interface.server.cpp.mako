#include <exception>
#include <map>
#include <nlohmann/json.hpp>
#include <sdbusplus/exception.hpp>
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

% if interface.enums:
namespace sdbusplus::common::${interface.cppNamespace()}
{
    % for e in interface.enums:
void to_json(nlohmann::json& j, ${interface.classname}::${e.name} e)
{
    j = sdbusplus::message::convert_to_string(e);
}

void from_json(const nlohmann::json& j, ${interface.classname}::${e.name}& e)
{
    auto value =
        sdbusplus::message::convert_from_string<${interface.classname}::${e.name}>(
            j.get<std::string>());
    if (!value)
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    e = *value;
}
    % endfor
} // namespace sdbusplus::common::${interface.cppNamespace()}
% endif

% if interface.properties and not any(p.is_variant() for p in interface.properties):
namespace sdbusplus::common::${interface.cppNamespace()}
{
void to_json(nlohmann::json& j, const ${interface.classname}::properties_t& v)
{
    j = nlohmann::json{
        % for p in interface.properties:
        {"${p.name}", v.${p.snake_case}},
        % endfor
    };
}

void from_json(const nlohmann::json& j, ${interface.classname}::properties_t& v)
{
    % for p in interface.properties:
    if (j.contains("${p.name}"))
    {
        j.at("${p.name}").get_to(v.${p.snake_case});
    }
    % endfor
}
} // namespace sdbusplus::common::${interface.cppNamespace()}
% endif

% if interface.properties and not any(p.is_variant() for p in interface.properties):
namespace sdbusplus::exception::details
{
template <has_interface_type T>
void extend(std::map<std::string, std::string>& extensions,
            std::string_view interface, const T& v)
{
    extensions[std::string(interface)] = nlohmann::json(v).dump();
}

template void extend<common::${interface.cppNamespacedClass()}::properties_t>(
    std::map<std::string, std::string>&, std::string_view,
    const common::${interface.cppNamespacedClass()}::properties_t&);
} // namespace sdbusplus::exception::details
% endif
