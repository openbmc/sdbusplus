struct ${event.CamelCase} final :
    public sdbusplus::exception::generated_event<${event.CamelCase}>
{
%if len(event.metadata) == 0:
    ${event.CamelCase}(
        std::source_location source = std::source_location::current()) :
        source(source), tid(pthread_self())
    {}
%else:
    ${event.CamelCase}(
        ${", ".join([
            f"metadata_t<\"{m.SNAKE_CASE}\">, {m.cppTypeParam(events.name)} {m.camelCase}_"
            for m in event.metadata ])},
        std::source_location source = std::source_location::current()) :
        ${", ".join([
            f"{m.camelCase}({m.camelCase}_)" for m in event.metadata ])},
        source(source)
    {}

%for m in event.metadata:
    const ${m.cppTypeParam(events.name)} ${m.camelCase};
%endfor

%endif
    int set_error(sd_bus_error*) const override;
    int set_error(SdBusInterface*, sd_bus_error*) const override;
    auto to_json() const -> nlohmann::json override;

    std::source_location source;
    pthread_t tid;

    static constexpr auto errName =
        "${events.name}.${event.name}";
    static constexpr auto errDesc =
        "${event.description}";
    static constexpr auto errWhat =
        "${events.name}.${event.name}: ${event.description}";
    static constexpr int errSeverity = ${event.severity};

    static constexpr auto errErrno = ${event.errno};
};
