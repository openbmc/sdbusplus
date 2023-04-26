    /** Get value of ${property.name}
     *  ${property.description.strip()}
     */
    auto ${property.camelCase}()
    {
        return proxy.template get_property<\
${property.cppTypeParam(interface.name)}>(ctx, "${property.name}");
    }

    /** Set value of ${property.name}
     *  ${property.description.strip()}
     */
    auto ${property.camelCase}(auto value)
    {
        return proxy.template set_property<\
${property.cppTypeParam(interface.name)}>(
            ctx, "${property.name}", std::forward<decltype(value)>(value));
    }
