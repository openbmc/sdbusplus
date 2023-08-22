<%
m_tag = method.snake_case + "_t"
m_param = method.parameter_types_as_list(interface)
m_return = method.cpp_return_type(interface)
%>\
    struct ${m_tag}
    {
        using value_types = std::tuple<${m_param}>;
        using return_type = ${m_return};
    };
