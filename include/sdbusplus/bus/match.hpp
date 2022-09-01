#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/slot.hpp>

#include <functional>
#include <memory>
#include <string>

namespace sdbusplus
{

namespace bus
{

namespace match
{

struct match : private sdbusplus::bus::details::bus_friend
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    match() = delete;
    match(const match&) = delete;
    match& operator=(const match&) = delete;
    match(match&&) = default;
    match& operator=(match&&) = default;
    ~match() = default;

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] handler - The callback for matches.
     *  @param[in] context - An optional context to pass to the handler.
     */
    match(sdbusplus::bus_t& bus, const char* match,
          sd_bus_message_handler_t handler, void* context = nullptr)
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_match(get_busp(bus), &slot, match, handler, context);

        _slot = std::move(slot);
    }
    match(sdbusplus::bus_t& bus, const std::string_view& _match,
          sd_bus_message_handler_t handler, void* context = nullptr) :
        match(bus, _match.data(), handler, context)
    {}

    using callback_t = std::function<void(sdbusplus::message_t&)>;

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    match(sdbusplus::bus_t& bus, const char* match, callback_t callback) :
        _callback(std::make_unique<callback_t>(std::move(callback)))
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_match(get_busp(bus), &slot, match, callCallback,
                         _callback.get());

        _slot = std::move(slot);
    }
    match(sdbusplus::bus_t& bus, const std::string_view& _match,
          callback_t callback) :
        match(bus, _match.data(), callback)
    {}

  private:
    slot_t _slot{};
    std::unique_ptr<callback_t> _callback = nullptr;

    // The callback is 'noexcept' because it is called from C code (sd-bus).
    // If it were to throw, this will cause undefined behavior so force noexcept
    // and we'll std::terminate instead.
    static int callCallback(sd_bus_message* m, void* context,
                            sd_bus_error* /*e*/) noexcept
    {
        auto c = static_cast<callback_t*>(context);
        message_t message{m};

        (*c)(message);

        return 0;
    }
};

/** Utilities for defining match rules based on the DBus specification */
namespace rules
{

using namespace std::string_literals;

namespace type
{

inline constexpr auto signal() noexcept
{
    return "type='signal',";
}
inline constexpr auto method() noexcept
{
    return "type='method',";
}
inline constexpr auto method_return() noexcept
{
    return "type='method_return',";
}
inline constexpr auto error() noexcept
{
    return "type='error',";
}

} // namespace type

inline constexpr auto sender(const std::string_view& s) noexcept
{
    return "sender='"s.append(s).append("',");
}
inline constexpr auto interface(const std::string_view& s) noexcept
{
    return "interface='"s.append(s).append("',");
}
inline constexpr auto member(const std::string_view& s) noexcept
{
    return "member='"s.append(s).append("',");
}
inline constexpr auto path(const std::string_view& s) noexcept
{
    return "path='"s.append(s).append("',");
}
inline constexpr auto path_namespace(const std::string_view& s) noexcept
{
    return "path_namespace='"s.append(s).append("',");
}
inline constexpr auto destination(const std::string_view& s) noexcept
{
    return "destination='"s.append(s).append("',");
}
inline auto argN(size_t n, const std::string_view& s) noexcept
{
    return "arg"s.append(std::to_string(n)).append("='").append(s).append("',");
}
inline auto argNpath(size_t n, const std::string_view& s) noexcept
{
    return "arg"s.append(std::to_string(n))
        .append("path='"s)
        .append(s)
        .append("',");
}
inline constexpr auto arg0namespace(const std::string_view& s) noexcept
{
    return "arg0namespace='"s.append(s).append("',");
}
inline constexpr auto eavesdrop() noexcept
{
    return "eavesdrop='true',";
}

inline constexpr auto nameOwnerChanged() noexcept
{
    return "type='signal',"
           "sender='org.freedesktop.DBus',"
           "member='NameOwnerChanged',";
}

inline constexpr auto interfacesAdded() noexcept
{
    return "type='signal',"
           "interface='org.freedesktop.DBus.ObjectManager',"
           "member='InterfacesAdded',";
}

inline constexpr auto interfacesRemoved() noexcept
{
    return "type='signal',"
           "interface='org.freedesktop.DBus.ObjectManager',"
           "member='InterfacesRemoved',";
}

inline constexpr std::string interfacesAdded(const std::string_view& p) noexcept
{
    return std::string(interfacesAdded()).append(path(p));
}

inline constexpr std::string
    interfacesRemoved(const std::string_view& p) noexcept
{
    return std::string(interfacesRemoved()).append(path(p));
}

inline auto propertiesChanged(const std::string_view& p,
                              const std::string_view& i) noexcept
{
    return std::string(type::signal())
        .append(path(p))
        .append(member("PropertiesChanged"s))
        .append(interface("org.freedesktop.DBus.Properties"s))
        .append(argN(0, i));
}

inline auto propertiesChangedNamespace(const std::string_view& p,
                                       const std::string_view& i) noexcept
{
    return std::string(type::signal())
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
inline auto nameOwnerChanged(const std::string_view& s) noexcept
{
    return std::string(nameOwnerChanged()).append(argN(0, s));
}

} // namespace rules
} // namespace match

using match_t = match::match;

} // namespace bus
} // namespace sdbusplus
