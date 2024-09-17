struct ${event.CamelCase} final :
    public sdbusplus::exception::generated_event<${event.CamelCase}>
{
    static constexpr auto errName =
        "${events.name}.${event.name}";
    static constexpr auto errDesc =
        "${event.description}";
    static constexpr auto errWhat =
        "${events.name}.${event.name}: ${event.description}";

    static constexpr auto errErrno = ${event.errno};
};
