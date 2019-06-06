#pragma once
#include <map>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/typed.hpp>
#include <string>
#include <utility>
#include <variant>

namespace sdbusplus
{
namespace common_calls
{
namespace properties
{

template <typename Property>
auto get(bus::bus& bus, const char* service, const char* object)
{
    using Req = typed::Types<std::string, std::string>;
    using Rsp = typed::Types<std::variant<Property>>;
    return typed::MethodCall<Req, Rsp>(bus.new_method_call(
        service, object, "org.freedesktop.DBus.Properties", "Get"));
}

template <typename Property>
Property get(bus::bus& bus, const char* service, const char* object,
             const char* interface, const char* property)
{
    std::variant<Property> ret;
    get<Property>(bus, service, object)
        .append(interface, property)
        .call()
        .read(ret);
    return std::move(std::get<Property>(ret));
}

template <typename Property>
auto set(bus::bus& bus, const char* service, const char* object)
{
    using Req = typed::Types<std::string, std::string, std::variant<Property>>;
    using Rsp = typed::Types<>;
    return typed::MethodCall<Req, Rsp>(bus.new_method_call(
        service, object, "org.freedesktop.DBus.Properties", "Set"));
}

template <typename Property>
void set(bus::bus& bus, const char* service, const char* object,
         const char* interface, const char* property, Property&& value)
{
    set<Property>(bus, service, object)
        .append(interface, property)
        .append(std::variant<Property>(std::forward<Property>(value)))
        .call();
}

template <typename... Properties>
using Map = std::map<std::string, std::variant<Properties...>>;

template <typename... Properties>
auto getAll(bus::bus& bus, const char* service, const char* object)
{
    using Req = typed::Types<std::string>;
    using Rsp = typed::Types<Map<Properties...>>;
    return typed::MethodCall<Req, Rsp>(bus.new_method_call(
        service, object, "org.freedesktop.DBus.Properties", "GetAll"));
}

template <typename... Properties>
Map<Properties...> getAll(bus::bus& bus, const char* service,
                          const char* object, const char* interface)
{
    Map<Properties...> ret;
    getAll<Properties...>(bus, service, object)
        .append(interface)
        .call()
        .read(ret);
    return ret;
}

} // namespace properties
} // namespace common_calls
} // namespace sdbusplus
