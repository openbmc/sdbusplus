<%
    def parameters(defaultValue=False):
        return ",\n            ".\
            join([ parameter(p, defaultValue) for p in signal.properties ])

    def parameter(p, defaultValue=False):
        r = "%s %s" % (p.cppTypeParam(interface.name), p.camelCase)
        if defaultValue:
            r += default_value(p)
        return r

    def parameters_as_list(pre="", post=""):
        return ", ".join([ "%s%s%s" % (pre, p.camelCase, post)
                for p in signal.properties ])

    def parameters_types_as_list():
        return ", ".join([ p.cppTypeParam(interface.name, full=True)
                for p in signal.properties ])

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
### Emit 'vtable'
###
    % elif ptype == 'vtable':
    vtable::signal("${signal.name}",
                   details::${interface_name()}::_signal_${signal.CamelCase }
                        .data()),
###
### Emit 'callback-cpp'
###
    % elif ptype == 'callback-cpp':
void ${interface_name()}::${ signal.camelCase }(
            ${ parameters() })
{
    auto& i = _${"_".join(interface.name.split('.'))}_interface;
    auto m = i.new_signal("${ signal.name }");

    m.append(${ parameters_as_list() });
    m.signal_send();
}

namespace details
{
namespace ${interface_name()}
{
static const auto _signal_${ signal.CamelCase } =
    % if len(signal.properties) == 0:
        utility::tuple_to_array(std::make_tuple('\0'));
    % else:
        utility::tuple_to_array(message::types::type_id<
                ${ parameters_types_as_list() }>());
    % endif
}
}
    % elif ptype == 'callback-hpp-includes':
        % for i in interface.enum_includes(signal.properties):
#include <${i}>
        % endfor
    % endif
