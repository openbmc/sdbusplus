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
    auto ${method.camelCase}(
            sdbusplus::async::context& ctx\
    % if len(method.parameters) != 0:
,
    % else:

    % endif
            ${method.get_parameters_str(interface)})
    {
        return proxy.template call<${method.returns_as_list(interface)}>(
            ctx, "${method.name}"\
    % if len(method.parameters) != 0:
,
    % else:

    % endif
            ${method.parameters_as_list()});
    }
