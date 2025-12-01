<%
    def parameters(defaultValue=False):
        return ",\n            ".\
            join([ parameter(p, defaultValue) for p in signal.properties ])

    def parameter(p, defaultValue=False):
        r = "%s %s" % (p.cppTypeParam(interface.name), p.camelCase)
        if defaultValue:
            r += p.default_value(interface.name)
        return r

    def parameters_as_list():
        return ", ".join([ p.camelCase for p in signal.properties ])

    def parameters_types_as_list():
        return ", ".join([ p.cppTypeParam(interface.name, full=True)
                for p in signal.properties ])
%>
###
### Emit 'header'
###
    % if ptype == 'header':
        /** @brief Send signal '${signal.name}'
         *
         *  ${ signal.description.strip() }
    % if len(signal.properties) != 0:
         *
        % for p in signal.properties:
         *  @param[in] ${p.camelCase} - ${p.description.strip()}
        % endfor
    % endif
         */
        void ${ signal.camelCase }(
            ${ parameters(True) });
###
### Emit 'callback-cpp'
###
    % elif ptype == 'callback-cpp':
void ${interface.classname}::${ signal.camelCase }(
            ${ parameters() })
{
    auto& i = _${interface.joinedName("_", "interface")};
    auto m = i.new_signal("${ signal.name }");

    m.append(${ parameters_as_list() });
    m.signal_send();
}

namespace details
{
namespace ${interface.classname}
{
static constexpr auto _signal_${ signal.CamelCase } =
        message::types::type_id<
                ${ parameters_types_as_list() }>();
}
}
    % endif
