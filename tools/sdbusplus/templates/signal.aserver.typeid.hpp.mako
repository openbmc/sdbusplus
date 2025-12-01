    static constexpr auto _signal_typeid_${signal.snake_case} =
        message::types::type_id<\
${ ", ".join( [ p.cppTypeParam(interface.name, full=True) for p in signal.properties ]) }>();
