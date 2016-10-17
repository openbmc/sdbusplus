<%
    def cpp_return_type():
        if len(method.returns) == 0:
            return "void"
        elif len(method.returns) == 1:
            return method.returns[0].typeName
        else:
            return "std::tuple<" + \
                   returns_as_list() + \
                   ">"

    def parameters(defaultValue=False):
        return ",\n            ".\
            join([ parameter(p, defaultValue) for p in method.parameters ])

    def parameters_as_local():
        return "{};\n    ".join([ parameter(p) for p in method.parameters ])

    def parameters_as_list():
        return ", ".join([ p.camelCase for p in method.parameters ])

    def parameters_types_as_list():
        return ", ".join([ p.typeName for p in method.parameters ])

    def parameter(p, defaultValue=False):
        r = "%s %s" % (p.typeName, p.camelCase)
        if defaultValue:
            r += default_value(p)
        return r

    def returns_as_list():
        return ", ".join([ r.typeName for r in method.returns ])

    def returns_as_tuple_index(tuple):
        return ", ".join([ "std::move(std::get<%d>(%s))" % (i,tuple) \
                for i in range(len(method.returns))])

    def default_value(p):
        if p.defaultValue != None:
            return " = " + str(p.defaultValue)
        else:
            return ""

    def interface_name():
        return interface.name.split('.').pop()
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
         *  @return ${r.camelCase}[${r.typeName}] - ${r.description.strip()}
        % endfor
    % endif
         */
        virtual ${cpp_return_type()} ${ method.camelCase }(
            ${ parameters() }) = 0;
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
                   _callback_${ method.CamelCase }),
###
### Emit 'callback-cpp'
###
    % elif ptype == 'callback-cpp':
int ${interface_name()}::_callback_${ method.CamelCase }(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    ### Need to add a ref to msg since we attached it to an sdbusplus::message.
    auto m = sdbusplus::message::message(sd_bus_message_ref(msg));

    % if len(method.parameters) != 0:
    ${parameters_as_local()}{};

    m.read(${parameters_as_list()});
    % endif

    auto o = static_cast<${interface_name()}*>(context);
    % if len(method.returns) != 0:
    auto r = \
    %endif
    o->${ method.camelCase }(${parameters_as_list()});

    auto reply = m.new_method_return();
    % if len(method.returns) == 0:
    // No data to append on reply.
    % elif len(method.returns) == 1:
    reply.append(std::move(r));
    % else:
    reply.append(${returns_as_tuple_index("r")});
    % endif

    reply.method_return();

    return 0;
}

namespace details {
namespace ${interface_name()} {
static const auto _param_${ method.CamelCase } =
    % if len(method.parameters) == 0:
        utility::tuple_to_array(std::make_tuple('\0'));
    % else:
        utility::tuple_to_array(message::types::type_id<
                ${ parameters_types_as_list() }>());
    % endif
static const auto _return_${ method.CamelCase } =
    % if len(method.parameters) == 0:
        utility::tuple_to_array(std::make_tuple('\0'));
    % else:
        utility::tuple_to_array(message::types::type_id<
                ${ returns_as_list() }>());
    % endif
}
}
    % endif
