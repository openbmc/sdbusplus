    static constexpr auto _signal_typeid_${signal.snake_case} =
% if len(signal.properties) == 0:
        utility::tuple_to_array(std::make_tuple('\0'));
% else:
        utility::tuple_to_array(message::types::type_id<\
${ ", ".join( [ p.cppTypeParam(interface.name, full=True) for p in signal.properties ]) }>());
% endif
