    vtable::method("${method.name}",
                   details::${interface.classname}::_param_${ method.CamelCase }.c_str(),
                   details::${interface.classname}::_return_${ method.CamelCase }.c_str(),
        % if method.cpp_flags:
                   _callback_${method.CamelCase},
                   ${method.cpp_flags}\
        % else:
                   _callback_${method.CamelCase}\
        % endif
),
