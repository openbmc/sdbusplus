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
    return nlohmann::json{ { errName, std::move(j) } };
}
