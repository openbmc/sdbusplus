#pragma once

#include <nlohmann/json.hpp>
#include <sdbusplus/exception.hpp>

#include <source_location>

namespace sdbusplus::sdbuspp
{

using register_hook = void (*)(const nlohmann::json&,
                               const std::source_location&);

void register_event(const std::string&, register_hook);

template <typename T>
struct register_event_helper
{
    static void hook()
    {
        register_event(T::errName, throw_event);
    }

    static void throw_event(const nlohmann::json& j,
                            const std::source_location& location)
    {
        throw T(j, location);
    }
};
} // namespace sdbusplus::sdbuspp
