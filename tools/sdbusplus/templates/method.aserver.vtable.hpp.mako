        vtable::method("${method.name}",
                       _method_typeid_p_${method.snake_case}.data(),
                       _method_typeid_r_${method.snake_case}.data(),
                       _callback_m_${method.snake_case}\
% if method.cpp_flags:
,
                       ${method.cpp_flags}\
% endif
),
