<%
p_name = property.snake_case
p_type = property.cppTypeParam(interface.name)

def p_value():
    if property.defaultValue == None:
        return "{}"

    value = str(property.defaultValue)
    enum_prefix = ""
    if property.is_enum():
        enum_prefix = f"{p_type}::"

    return f" = {enum_prefix}{value}"

%>\
    ${p_type} ${p_name}_${p_value()};
