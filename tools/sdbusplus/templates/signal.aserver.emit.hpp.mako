<%
    def parameters():
        return ",\n            ".\
            join([ parameter(p) for p in signal.properties ])

    def parameter(p):
        r = "%s %s%s" % \
            (p.cppTypeParam(interface.name), p.camelCase, default_value(p))
        return r

    def parameters_as_list():
        return ", ".join([ p.camelCase for p in signal.properties ])

    def default_value(p):
        if p.defaultValue != None:
            return " = " + str(p.defaultValue)
        else:
            return ""
%>\
    /** @brief Send signal '${signal.name}'
     *
     *  ${ signal.description.strip() }
% if len(signal.properties) != 0:
     *
    % for p in signal.properties:
     *  @param[in] ${p.camelCase} - ${p.description.strip()}
    % endfor
% endif
     *
     */
    void ${signal.camelCase}(${parameters()})
    {
        auto m = _${interface.joinedName("_", "interface")}
                     .new_signal(\
"${signal.name}");

        m.append(${parameters_as_list()});
        m.signal_send();
    }
