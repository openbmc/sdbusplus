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

%if len(event.metadata) > 0:
    ${event.CamelCase}() = delete;
    ${event.CamelCase}(
        ${", ".join([
            f"utility::consteval_string<\"{m.SNAKE_CASE}\">, {m.cppTypeParam(events.name)} {m.camelCase}_"
            for m in event.metadata ])}) :
        ${", ".join([
            f"{m.camelCase}({m.camelCase}_)" for m in event.metadata ])}
    {}

%for m in event.metadata:
    ${m.cppTypeParam(events.name)} ${m.camelCase};
%endfor
%endif
};
