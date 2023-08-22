    static constexpr auto _method_typeid_p_${method.snake_case} =
        utility::tuple_to_array(\
% if len(method.parameters) == 0:
std::make_tuple('\0')\
% else:
message::types::type_id<${method.parameter_types_as_list(interface)}>()\
% endif
);

    static constexpr auto _method_typeid_r_${method.snake_case} =
        utility::tuple_to_array(\
% if len(method.returns) == 0:
std::make_tuple('\0')\
% else:
message::types::type_id<${method.returns_as_list(interface)}>()\
% endif
);
