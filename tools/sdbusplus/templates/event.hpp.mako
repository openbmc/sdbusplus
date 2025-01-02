struct ${event.CamelCase} final :
    public sdbusplus::exception::generated_event<${event.CamelCase}>
{
    ${event.CamelCase}(const nlohmann::json&, const std::source_location&);
%if len(event.metadata) == 0:
    ${event.CamelCase}(
        const std::source_location& source = std::source_location::current()) :
        source_file(source.file_name()), source_func(source.function_name()),
        source_line(source.line()), source_column(source.column()),
        pid(getpid())
    {}
%else:
    ${event.CamelCase}(
        ${", ".join([
            f"metadata_t<\"{m.SNAKE_CASE}\">, {m.cppTypeParam(events.name)} {m.camelCase}_"
            for m in event.metadata ])},
        const std::source_location& source = std::source_location::current()) :
        ${", ".join([
            f"{m.camelCase}({m.camelCase}_)" for m in event.metadata ])},
        source_file(source.file_name()), source_func(source.function_name()),
        source_line(source.line()), source_column(source.column()),
        pid(getpid())
    {}

%for m in event.metadata:
    ${m.cppTypeParam(events.name)} ${m.camelCase};
%endfor

%endif
    int set_error(sd_bus_error*) const override;
    int set_error(SdBusInterface*, sd_bus_error*) const override;
    auto to_json() const -> nlohmann::json override;

    std::string source_file;
    std::string source_func;
    size_t source_line;
    size_t source_column;
    pid_t pid;

    static constexpr auto errName =
        "${events.name}.${event.name}";
    static constexpr auto errDesc =
        "${event.description}";
    static constexpr auto errWhat =
        "${events.name}.${event.name}: ${event.description}";
    static constexpr int errSeverity = ${event.syslog_sev};

    static constexpr auto errErrno = ${event.errno};
};

namespace details
{
void register_${event.CamelCase}();
[[gnu::constructor]] inline void force_register_${event.CamelCase}()
{
    register_${event.CamelCase}();
}
} // namespace details
