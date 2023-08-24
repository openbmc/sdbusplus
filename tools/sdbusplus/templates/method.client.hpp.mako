    /** @brief ${ method.name }
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
    auto ${method.snake_case}(\
    % if len(method.parameters) != 0:
${method.get_parameters_str(interface, join_str=", ")}\
    % else:
    % endif
)
    {
        return proxy.template call<\
${method.returns_as_list(interface)}>(context(), "${method.name}"\
    % if len(method.parameters) != 0:
, ${method.parameters_as_list()}\
    % else:
    % endif
);
    }
