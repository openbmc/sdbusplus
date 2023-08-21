    static constexpr auto _property_typeid_${property.snake_case} =
        utility::tuple_to_array(message::types::type_id<\
${property.cppTypeParam(interface.name)}>());
