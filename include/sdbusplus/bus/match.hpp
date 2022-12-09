#pragma once

#include <systemd/sd-bus.h>

#include <function2/function2.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/slot.hpp>
#include <stdplus/numeric/str.hpp>
#include <stdplus/str/cat.hpp>
#include <stdplus/zstring.hpp>

#include <memory>
#include <string>

namespace sdbusplus
{

namespace message
{
struct message;
}
using message_t = message::message;

namespace bus
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
    match(sdbusplus::bus_t& bus, stdplus::const_zstring match,
          sd_bus_message_handler_t handler, void* context = nullptr);

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    using callback_t = fu2::unique_function<void(sdbusplus::message_t&)>;
    match(sdbusplus::bus_t& bus, stdplus::const_zstring match, callback_t&& cb);

  private:
    std::unique_ptr<callback_t> cb;
    slot_t slot;
};

/** Utilities for defining match rules based on the DBus specification */
namespace rules
{

using namespace std::string_literals;
using namespace std::string_view_literals;
using stdplus::strCat;

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
    return strCat("sender='"sv, s, "',"sv);
}
constexpr auto interface(std::string_view s) noexcept
{
    return strCat("interface='"sv, s, "',"sv);
}
constexpr auto member(std::string_view s) noexcept
{
    return strCat("member='"sv, s, "',"sv);
}
constexpr auto path(std::string_view s) noexcept
{
    return strCat("path='"sv, s, "',"sv);
}
constexpr auto path_namespace(std::string_view s) noexcept
{
    return strCat("path_namespace='"sv, s, "',"sv);
}
constexpr auto destination(std::string_view s) noexcept
{
    return strCat("destination='"sv, s, "',"sv);
}
constexpr auto argN(size_t n, std::string_view s) noexcept
{
    return strCat("arg"sv, stdplus::to_string(n), "='"sv, s, "',"sv);
}
constexpr auto argNpath(size_t n, std::string_view s) noexcept
{
    return strCat("arg"sv, stdplus::to_string(n), "path='"sv, s, "',"sv);
}
constexpr auto arg0namespace(std::string_view s) noexcept
{
    return strCat("arg0namespace='"sv, s, "',"sv);
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

constexpr auto interfacesAdded(std::string_view p) noexcept
{
    return strCat(interfacesAdded(), path(p));
}

constexpr auto interfacesRemoved() noexcept
{
    return "type='signal',interface='org.freedesktop.DBus.ObjectManager',member='InterfacesRemoved',"s;
}

constexpr auto interfacesRemoved(std::string_view p) noexcept
{
    return strCat(interfacesRemoved(), path(p));
}

constexpr auto propertiesChanged(std::string_view p,
                                 std::string_view i) noexcept
{
    return strCat(type::signal(), path(p), member("PropertiesChanged"sv),
                  interface("org.freedesktop.DBus.Properties"sv), argN(0, i));
}

constexpr auto propertiesChangedNamespace(std::string_view p,
                                          std::string_view i) noexcept
{
    return strCat(
        type::signal(), path_namespace(p), member("PropertiesChanged"sv),
        interface("org.freedesktop.DBus.Properties"sv), arg0namespace(i));
}
/**
 * @brief Constructs a NameOwnerChanged match string for a service name
 *
 * @param[in] s - Service name
 *
 * @return NameOwnerChanged match string for a service name
 */
constexpr auto nameOwnerChanged(std::string_view s) noexcept
{
    return strCat(nameOwnerChanged(), argN(0, s));
}

} // namespace rules
} // namespace match

using match_t = match::match;

} // namespace bus
} // namespace sdbusplus
