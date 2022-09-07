<%
    def parameters_as_arg_list():
        return ", ".join([ parameter(p, ref="&&") for p in method.parameters ])

    def parameters_as_list(transform=lambda p: p.camelCase):
        return ", ".join([ transform(p) for p in method.parameters ])

    def parameters_types_as_list():
        return ", ".join([ p.cppTypeParam(interface.name, full=True)
                for p in method.parameters ])

    def parameter(p, defaultValue=False, ref=""):
        r = "%s%s %s" % (p.cppTypeParam(interface.name), ref, p.camelCase)
        if defaultValue:
            r += default_value(p)
        return r

    def default_value(p):
        if p.defaultValue != None:
            return " = " + str(p.defaultValue)
        else:
            return ""

    def interface_name():
        return interface.name.split('.').pop()

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
###
### Emit 'header'
###
    % if ptype == 'header':
        /** @brief Implementation for ${ method.name }
         *  ${ method.description.strip() }
    % if len(method.parameters) != 0:
         *
        % for p in method.parameters:
         *  @param[in] ${p.camelCase} - ${p.description.strip()}
        % endfor
    % endif
    % if len(method.returns) != 0:
         *
        % for r in method.returns:
         *  @return ${r.camelCase}[${r.cppTypeParam(interface.name)}] \
- ${r.description.strip()}
        % endfor
    % endif
         */
        virtual ${method.cpp_return_type(interface)} ${ method.camelCase }(
            ${ method.get_parameters_str(interface) }) = 0;
###
### Emit 'callback-header'
###
    % elif ptype == 'callback-header':
        /** @brief sd-bus callback for ${ method.name }
         */
        static int _callback_${ method.CamelCase }(
            sd_bus_message*, void*, sd_bus_error*);
###
### Emit 'vtable'
###
    % elif ptype == 'vtable':
    vtable::method("${method.name}",
                   details::${interface_name()}::_param_${ method.CamelCase }
                        .data(),
                   details::${interface_name()}::_return_${ method.CamelCase }
                        .data(),
        % if method.cpp_flags:
                   _callback_${method.CamelCase},
                   ${method.cpp_flags}),
        % else:
                   _callback_${method.CamelCase}),
        % endif
###
### Emit 'callback-cpp'
###
    % elif ptype == 'callback-cpp':
int ${interface_name()}::_callback_${ method.CamelCase }(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    auto o = static_cast<${interface_name()}*>(context);

    % if method.errors:
    try
    % endif
    {
        auto cb = [o](${parameters_as_arg_list()}) {
            return o->${ method.camelCase }(${parameters_as_list()});
        };
        return sdbusplus::sdbuspp::method_callback<\
${'true' if len(method.returns) > 1 else 'false'}>(
                msg, o->_intf, error, sdbusplus::sdbuspp::fview(cb));
    }
    % for e in method.errors:
    catch(const sdbusplus::${error_namespace(e)}::${error_name(e)}& e)
    {
        return o->_intf->sd_bus_error_set(error, e.name(), e.description());
    }
    % endfor

    return 0;
}

namespace details
{
namespace ${interface_name()}
{
static const auto _param_${ method.CamelCase } =
    % if len(method.parameters) == 0:
        utility::tuple_to_array(std::make_tuple('\0'));
    % else:
        utility::tuple_to_array(message::types::type_id<
                ${ parameters_types_as_list() }>());
    % endif
static const auto _return_${ method.CamelCase } =
    % if len(method.returns) == 0:
        utility::tuple_to_array(std::make_tuple('\0'));
    % else:
        utility::tuple_to_array(message::types::type_id<
                ${ method.returns_as_list(interface, full=True) }>());
    % endif
}
}
    % elif ptype == 'callback-cpp-includes':
        % for e in method.errors:
#include <${error_include(e)}>
        % endfor
    % elif ptype == 'callback-hpp-includes':
        % for i in interface.enum_includes(method.returns + method.parameters):
#include <${i}>
        % endfor
    % endif
