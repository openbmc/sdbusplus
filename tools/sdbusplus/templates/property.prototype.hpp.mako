<%
    def error_namespace(e):
        n = e.split('.');
        n.pop(); # Remove error name.

        n = map((lambda x: interface.name if x == "self" else x), n);
        return '::'.join('.'.join(n).split('.'));

    def error_name(e):
        return e.split('.').pop();

    def error_include(e):
        l = error_namespace(e).split('::')
        l.pop() # Remove "Error"
        return '/'.join(l) + '/error.hpp';
%>
% if ptype == 'callback-cpp':
auto ${interface.classname}::${property.camelCase}() const ->
        ${property.cppTypeParam(interface.name)}
{
    return _${property.camelCase};
}

int ${interface.classname}::_callback_get_${property.name}(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* reply, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<${interface.classname}*>(context);

    % if property.errors:
    try
    % endif
    {
        return sdbusplus::sdbuspp::property_callback(
                reply, o->_intf, error,
                std::function(
                    [=]()
                    {
                        return o->${property.camelCase}();
                    }
                ));
    }
    % for e in property.errors:
    catch(const sdbusplus::${error_namespace(e)}::${error_name(e)}& e)
    {
        return o->_intf->sd_bus_error_set(error, e.name(), e.description());
    }
    % endfor
}

auto ${interface.classname}::${property.camelCase}(${property.cppTypeParam(interface.name)} value,
                                         bool skipSignal) ->
        ${property.cppTypeParam(interface.name)}
{
    if (_${property.camelCase} != value)
    {
        _${property.camelCase} = value;
        if (!skipSignal)
        {
            _${interface.joinedName("_", "interface")}.property_changed("${property.name}");
        }
    }

    return _${property.camelCase};
}

auto ${interface.classname}::${property.camelCase}(${property.cppTypeParam(interface.name)} val) ->
        ${property.cppTypeParam(interface.name)}
{
    return ${property.camelCase}(val, false);
}

% if 'const' not in property.flags and 'readonly' not in property.flags:
int ${interface.classname}::_callback_set_${property.name}(
        sd_bus* /*bus*/, const char* /*path*/, const char* /*interface*/,
        const char* /*property*/, sd_bus_message* value, void* context,
        sd_bus_error* error)
{
    auto o = static_cast<${interface.classname}*>(context);

    % if property.errors:
    try
    % endif
    {
        return sdbusplus::sdbuspp::property_callback(
                value, o->_intf, error,
                std::function(
                    [=](${property.cppTypeParam(interface.name)}&& arg)
                    {
                        o->${property.camelCase}(std::move(arg));
                    }
                ));
    }
    % for e in property.errors:
    catch(const sdbusplus::${error_namespace(e)}::${error_name(e)}& e)
    {
        return o->_intf->sd_bus_error_set(error, e.name(), e.description());
    }
    % endfor
}
% endif

namespace details
{
namespace ${interface.classname}
{
static const auto _property_${property.name} =
    utility::tuple_to_array(message::types::type_id<
            ${property.cppTypeParam(interface.name, full=True)}>());
}
}
% elif ptype == 'callback-cpp-includes':
        % for e in property.errors:
#include <${error_include(e)}>
        % endfor
% endif
