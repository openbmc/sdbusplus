    /** Get value of ${property.name}
     *  ${property.description.strip()}
     */
    auto ${property.snake_case}()
    {
        return proxy.template get_property<\
${property.cppTypeParam(interface.name)}>(context(), "${property.name}");
    }

% if 'const' not in property.flags and 'readonly' not in property.flags:
    /** Set value of ${property.name}
     *  ${property.description.strip()}
     */
    auto ${property.snake_case}(auto value)
    {
        return proxy.template set_property<\
${property.cppTypeParam(interface.name)}>(
            context(), "${property.name}", std::forward<decltype(value)>(value));
    }
% endif
