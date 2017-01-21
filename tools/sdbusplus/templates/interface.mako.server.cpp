#include <algorithm>
#include <sdbusplus/server.hpp>
#include <sdbusplus/exception.hpp>
#include <${"/".join(interface.name.split('.') + [ 'server.hpp' ])}>
% for m in interface.methods:
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

sdbusplus::server::transaction::Transaction ${classname}::t;

${classname}::${classname}(bus::bus& bus, const char* path)
        : _${interface_instance()}(
                bus, path, _interface, _vtable, this)
{
    t.bus = &bus;
}

    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for s in interface.signals:
${ s.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for p in interface.properties:
auto ${classname}::${p.camelCase}() const ->
        ${p.cppTypeParam(interface.name)}
{
    return _${p.camelCase};
}

int ${classname}::_callback_get_${p.name}(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    using sdbusplus::server::binding::details::convertForMessage;

    try
    {
        auto m = message::message(reply);

        auto o = static_cast<${classname}*>(context);
        m.append(convertForMessage(o->${p.camelCase}()));
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

auto ${classname}::${p.camelCase}(${p.cppTypeParam(interface.name)} value) ->
        ${p.cppTypeParam(interface.name)}
{
    if (_${p.camelCase} != value)
    {
        _${p.camelCase} = value;
        _${interface_instance()}.property_changed("${p.name}");
    }

    return _${p.camelCase};
}

int ${classname}::_callback_set_${p.name}(
        sd_bus* bus, const char* path, const char* interface,
        const char* property, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    try
    {
        auto m = message::message(value);

        auto o = static_cast<${classname}*>(context);

        ${p.cppTypeMessage(interface.name)} v{};
        m.read(v);
    % if p.is_enum():
        o->${p.camelCase}(${p.enum_namespace(interface.name)}\
convert${p.enum_name(interface.name)}FromString(v));
    % else:
        o->${p.camelCase}(v);
    % endif
    }
    catch(sdbusplus::internal_exception_t& e)
    {
        sd_bus_error_set_const(error, e.name(), e.description());
        return -EINVAL;
    }

    return true;
}

namespace details
{
namespace ${classname}
{
static const auto _property_${p.name} =
    utility::tuple_to_array(message::types::type_id<
            ${p.cppTypeMessage(interface.name)}>());
}
}
    % endfor

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

auto ${classname}::convert${e.name}FromString(std::string& s) ->
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

std::string convertForMessage(${classname}::${e.name} v)
{
    auto i = std::find_if(
            std::begin(mapping${classname}${e.name}),
            std::end(mapping${classname}${e.name}),
            [v](auto& e){ return v == std::get<1>(e); });
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
                     _callback_set_${p.name},
                     vtable::property_::emits_change),
    % endfor
    vtable::end()
};

} // namespace server
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
