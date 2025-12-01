        vtable::property("${property.name}",
                         _property_typeid_${property.snake_case}.c_str(),
                         _callback_get_${property.snake_case},
% if 'const' not in property.flags and 'readonly' not in property.flags:
                         _callback_set_${property.snake_case},
% endif
% if not property.cpp_flags:
                         vtable::property_::emits_change\
% else:
                         ${property.cpp_flags}\
% endif
),
