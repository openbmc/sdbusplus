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

    def parameters(m, defaultValue=False):
        return ",\n            ".\
            join([ parameter(p, defaultValue) for p in m.parameters ])

    def parameter(p, defaultValue=False):
        r = "%s %s" % (p.typeName, p.camelCase)
        if defaultValue:
            r += default_value(p)
        return r

    def default_value(p):
        if p.defaultValue != None:
            return " = " + str(p.defaultValue)
        else:
            return ""
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
            ${ parameters(method, True) }) = 0;
###
### Emit 'callback-header'
###
    % elif ptype == 'callback-header':
        /** @brief sd-bus callback for ${ method.name }
         */
        static int _callback_${ method.CamelCase }(
            sd_bus_message*, void*, sd_bus_error*);
    % endif
