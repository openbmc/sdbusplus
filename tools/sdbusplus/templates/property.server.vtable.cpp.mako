    vtable::property("${property.name}",
                     details::${interface.classname}::_property_${property.name}.c_str(),
                     _callback_get_${property.name},
        % if 'const' not in property.flags and 'readonly' not in property.flags:
                     _callback_set_${property.name},
        % endif
        % if not property.cpp_flags:
                     vtable::property_::emits_change\
        % else:
                     ${property.cpp_flags}\
        % endif
),
