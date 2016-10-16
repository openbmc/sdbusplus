<%
    def cpp_return_type():
        if len(method.returns) == 0:
            return "void"
        elif len(method.returns) == 1:
            return method.returns[0].typeName
        else:
            return "std::tuple<" + \
                   ", ".join([ r.typeName for r in method.returns ]) + \
                   ">"

    def parameters(defaultValue=False):
        return ",\n            ".\
            join([ parameter(p, defaultValue) for p in method.parameters ])

    def parameters_as_local():
        return "{};\n    ".join([ parameter(p) for p in method.parameters ])

    def parameters_as_list():
        return ", ".join([ p.camelCase for p in method.parameters ])

    def parameter(p, defaultValue=False):
        r = "%s %s" % (p.typeName, p.camelCase)
        if defaultValue:
            r += default_value(p)
        return r

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
### Emit 'callback-cpp'
###
    % elif ptype == 'callback-cpp':
int ${interface_name()}::_callback_${ method.CamelCase }(
        sd_bus_message* msg, void* context, sd_bus_error* error)
{
    auto m = sdbusplus::message::message(msg);
    ### Need to add a ref to msg since we attached it to an sdbusplus::message.
    sd_bus_message_ref(msg);

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

    sdbusplus::bus::method_return(reply);

    return 0;
}
    % endif
