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

auto ${event.CamelCase}::getRedfishArgs() const -> nlohmann::ordered_json
{
    nlohmann::ordered_json oj = { };
% for m in event.metadata:
    % if not m.primary:
        <% continue %>
    % endif
    % if m.typeName == "object_path":
    oj["${m.SNAKE_CASE}"] = ${m.camelCase}.str;
    % elif m.is_enum():
    oj["${m.SNAKE_CASE}"] = sdbusplus::message::convert_to_string(${m.camelCase});
    % else:
    oj["${m.SNAKE_CASE}"] = ${m.camelCase};
    % endif
% endfor

    return nlohmann::ordered_json{ { errName, std::move(oj) } };
}

${event.CamelCase}::${event.CamelCase}(
    const nlohmann::json& j, const std::source_location& s)
{
    const nlohmann::json& self = j.at(errName);

% for m in event.metadata:
    % if m.typeName == "object_path":
    ${m.camelCase} = self.at("${m.SNAKE_CASE}").get<std::string>();
    % elif m.is_enum():
    if (auto enum_value =
            sdbusplus::message::convert_from_string<decltype(${m.camelCase})>(
                self.at("${m.SNAKE_CASE}"));
        enum_value.has_value())
    {
        ${m.camelCase} = enum_value.value();
    }
    else
    {
        throw sdbusplus::exception::InvalidEnumString();
    }
    % else:
    ${m.camelCase} = self.at("${m.SNAKE_CASE}");
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
        source_file = self.at("FILE");
        source_func = self.at("FUNCTION");
        source_line = self.at("LINE");
        source_column = self.at("COLUMN");
        pid = self.at("PID");
    }

}

namespace details
{
void register_${event.CamelCase}()
{
    sdbusplus::sdbuspp::register_event_helper<${event.CamelCase}>::hook();
}
}
