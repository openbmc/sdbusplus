int ${event.CamelCase}::set_error(sd_bus_error* e) const
{
    if (jsonString.empty())
    {
        jsonString = to_json().dump();
    }

    return sd_bus_error_set(e, errName, jsonString.c_str());
}

int ${event.CamelCase}::set_error(SdBusInterface* i, sd_bus_error* e) const
{
    if (jsonString.empty())
    {
        jsonString = to_json().dump();
    }

    return i->sd_bus_error_set(e, errName, jsonString.c_str());
}

auto ${event.CamelCase}::to_json() const -> nlohmann::json
{
    nlohmann::json j = { };
% for m in event.metadata:
    % if m.typeName == "object_path":
    j["${m.SNAKE_CASE}"] = ${m.camelCase}.str;
    % elif m.is_enum():
    j["${m.SNAKE_CASE}"] = sdbusplus::message::convert_to_string(${m.camelCase});
    % else:
    j["${m.SNAKE_CASE}"] = ${m.camelCase};
    % endif
% endfor

    // Add common source and pid info.
    nlohmann::json source_info = {};
    source_info["FILE"] = source_file;
    source_info["FUNCTION"] = source_func;
    source_info["LINE"] = source_line;
    source_info["COLUMN"] = source_column;
    source_info["PID"] = pid;
    j["_SOURCE"] = source_info;

    return nlohmann::json{ { errName, std::move(j) } };
}

${event.CamelCase}::${event.CamelCase}(
    const nlohmann::json& j, const std::source_location& s)
{
    const nlohmann::json& self = j[errName];

% for m in event.metadata:
    % if m.typeName == "object_path":
    ${m.camelCase} = j["${m.SNAKE_CASE}"].get<std::string>();
    % elif m.is_enum():
    ${m.camelCase} =
        sdbusplus::message::convert_from_string<decltype(${m.camelCase})>(
            j["${m.SNAKE_CASE}"]
        ).value();
    % else:
    ${m.camelCase} = j["${m.SNAKE_CASE}"];
    % endif
% endfor

    if (!self.contains("_SOURCE"))
    {
        source_file = s.file_name();
        source_func = s.function_name();
        source_line = s.line();
        source_column = s.column();
        pid = getpid();
    }
    else
    {
        source_file = self["FILE"];
        source_func = self["FUNCTION"];
        source_line = self["LINE"];
        source_column = self["COLUMN"];
        pid = self["PID"];
    }

}

[[gnu::constructor]] static void register_${event.CamelCase}()
{
    sdbusplus::sdbuspp::register_event_helper<${event.CamelCase}>::hook();
}
