#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/slot.hpp>

#include <functional>
#include <memory>
#include <string>

namespace sdbusplus::bus
{

namespace match
{

struct match : private sdbusplus::bus::details::bus_friend
{
    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] handler - The callback for matches.
     *  @param[in] context - An optional context to pass to the handler.
     */
    match(sdbusplus::bus_t& bus, const char* _match,
          sd_bus_message_handler_t handler, void* context = nullptr);
    inline match(sdbusplus::bus_t& bus, const std::string& _match,
                 sd_bus_message_handler_t handler, void* context = nullptr) :
        match(bus, _match.c_str(), handler, context)
    {}

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    using callback_t = std::function<void(sdbusplus::message_t&)>;
    match(sdbusplus::bus_t& bus, const char* _match, callback_t callback);
    inline match(sdbusplus::bus_t& bus, const std::string& _match,
                 callback_t callback) :
        match(bus, _match.c_str(), std::move(callback))
    {}

  private:
    std::unique_ptr<callback_t> _callback;
    slot_t _slot;
};

/** Utilities for defining match rules based on the DBus specification */
namespace rules
{

using namespace std::string_literals;

namespace type
{

constexpr auto signal() noexcept
{
    return "type='signal',"s;
}
constexpr auto method() noexcept
{
    return "type='method',"s;
}
constexpr auto method_return() noexcept
{
    return "type='method_return',"s;
}
constexpr auto error() noexcept
{
    return "type='error',"s;
}

} // namespace type

constexpr auto sender(std::string_view s) noexcept
{
    return "sender='"s.append(s).append("',");
}
constexpr auto interface(std::string_view s) noexcept
{
    return "interface='"s.append(s).append("',");
}
constexpr auto member(std::string_view s) noexcept
{
    return "member='"s.append(s).append("',");
}
constexpr auto path(std::string_view s) noexcept
{
    return "path='"s.append(s).append("',");
}
constexpr auto path_namespace(std::string_view s) noexcept
{
    return "path_namespace='"s.append(s).append("',");
}
constexpr auto destination(std::string_view s) noexcept
{
    return "destination='"s.append(s).append("',");
}
inline auto argN(size_t n, std::string_view s) noexcept
{
    return "arg"s.append(std::to_string(n)).append("='").append(s).append("',");
}
inline auto argNpath(size_t n, std::string_view s) noexcept
{
    return "arg"s.append(std::to_string(n))
        .append("path='"s)
        .append(s)
        .append("',");
}
constexpr auto arg0namespace(std::string_view s) noexcept
{
    return "arg0namespace='"s.append(s).append("',");
}
constexpr auto eavesdrop() noexcept
{
    return "eavesdrop='true',"s;
}

constexpr auto nameOwnerChanged() noexcept
{
    return "type='signal',sender='org.freedesktop.DBus',member='NameOwnerChanged',"s;
}

constexpr auto interfacesAdded() noexcept
{
    return "type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesAdded',"s;
}

constexpr auto interfacesRemoved() noexcept
{
    return "type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved',"s;
}

constexpr auto interfacesAdded(std::string_view p) noexcept
{
    return interfacesAdded().append(path(p));
}

inline auto interfacesAddedAtPath(std::string_view p) noexcept
{
    return interfacesAdded().append(argNpath(0, p));
}

constexpr auto interfacesRemoved(std::string_view p) noexcept
{
    return interfacesRemoved().append(path(p));
}

inline auto interfacesRemovedAtPath(std::string_view p) noexcept
{
    return interfacesRemoved().append(argNpath(0, p));
}

inline auto propertiesChanged(std::string_view p, std::string_view i) noexcept
{
    return type::signal()
        .append(path(p))
        .append(member("PropertiesChanged"s))
        .append(interface("org.freedesktop.DBus.Properties"s))
        .append(argN(0, i));
}

constexpr auto propertiesChangedNamespace(std::string_view p,
                                          std::string_view i) noexcept
{
    return type::signal()
        .append(path_namespace(p))
        .append(member("PropertiesChanged"s))
        .append(interface("org.freedesktop.DBus.Properties"s))
        .append(arg0namespace(i));
}
/**
 * @brief Constructs a NameOwnerChanged match string for a service name
 *
 * @param[in] s - Service name
 *
 * @return NameOwnerChanged match string for a service name
 */
inline auto nameOwnerChanged(std::string_view s) noexcept
{
    return nameOwnerChanged().append(argN(0, s));
}

} // namespace rules
} // namespace match

using match_t = match::match;

} // namespace sdbusplus::bus
