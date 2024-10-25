    static constexpr auto _method_typeid_p_${method.snake_case} =
        utility::tuple_to_array(\
message::types::type_id<${method.parameter_types_as_list(interface)}>()\
);

    static constexpr auto _method_typeid_r_${method.snake_case} =
        utility::tuple_to_array(\
message::types::type_id<${method.returns_as_list(interface)}>()\
);
