<%
p_tag = property.snake_case + "_t"
p_type = property.cppTypeParam(interface.name)
%>\
    struct ${p_tag}
    {
        using value_type = ${p_type};
        ${p_tag}() = default;
        explicit ${p_tag}(value_type) {}
    };
