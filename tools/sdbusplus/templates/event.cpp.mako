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
    j["${m.SNAKE_CASE}"] = ${m.camelCase};
% endfor

    // Add common source and pid info.
    nlohmann::json source_info = {};
    source_info["FILE"] = source_file;
    source_info["FUNCTION"] = source_func;
    source_info["LINE"] = source_line;
    source_info["COLUMN"] = source_column;
    source_info["PID"] = pid;
    j["_SOURCE"] = source_info;

    if (!extensions.empty())
    {
        for (const auto& [interface, dump] : extensions)
        {
            j["_EXTENSIONS"][interface] = nlohmann::json::parse(dump);
        }
    }

    return nlohmann::json{ { errName, std::move(j) } };
}

${event.CamelCase}::${event.CamelCase}(
    const nlohmann::json& j, const std::source_location& s)
{
    const nlohmann::json& self = j.at(errName);

% for m in event.metadata:
    self.at("${m.SNAKE_CASE}").get_to(${m.camelCase});
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
        const nlohmann::json& src = self.at("_SOURCE");
        source_file = src.at("FILE");
        source_func = src.at("FUNCTION");
        source_line = src.at("LINE");
        source_column = src.at("COLUMN");
        pid = src.at("PID");
    }

}

namespace details
{
void register_${event.CamelCase}()
{
    sdbusplus::sdbuspp::register_event_helper<${event.CamelCase}>::hook();
}
}
