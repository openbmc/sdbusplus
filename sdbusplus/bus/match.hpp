#pragma once

#include <functional>
#include <memory>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/slot.hpp>
#include <string>

namespace sdbusplus
{

namespace bus
{

namespace match
{

struct match
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
    match(sdbusplus::bus::bus& bus, const char* match,
          sd_bus_message_handler_t handler, void* context = nullptr,
          bool asyncCall = false,
          sd_bus_message_handler_t installHandler = nullptr) :
        _slot(nullptr)
    {
        sd_bus_slot* slot = nullptr;
        int ret = 0;
        if (asyncCall)
        {
            ret = sd_bus_add_match_async(bus.get(), &slot, match, handler,
                                         installHandler, context);
        }
        else
        {
            ret = sd_bus_add_match(bus.get(), &slot, match, handler, context);
        }
        if (ret < 0)
        {
            throw sdbusplus::exception::SdBusError(-EIO,
                                                   "ERROR in add match rule");
        }

        _slot = decltype(_slot){slot};
    }
    match(sdbusplus::bus::bus& bus, const std::string& _match,
          sd_bus_message_handler_t handler, void* context = nullptr,
          bool asyncCall = false,
          sd_bus_message_handler_t installHandler = nullptr) :
        match(bus, _match.c_str(), handler, context, asyncCall, installHandler)
    {
    }

    using callback_t = std::function<void(sdbusplus::message::message&)>;
    using contextPair_t =
        std::pair<std::unique_ptr<callback_t>, std::unique_ptr<callback_t>>;

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    match(sdbusplus::bus::bus& bus, const char* match, callback_t callback,
          bool asyncCall = false, callback_t installCallback = nullptr) :
        _slot(nullptr),
        callbackContext(std::make_unique<contextPair_t>(
            std::make_pair<std::unique_ptr<callback_t>,
                           std::unique_ptr<callback_t>>(
                std::make_unique<callback_t>(std::move(callback)),
                installCallback
                    ? std::make_unique<callback_t>(std::move(installCallback))
                    : nullptr)))
    {
        sd_bus_slot* slot = nullptr;
        int ret = 0;
        if (asyncCall)
        {
            ret = sd_bus_add_match_async(
                bus.get(), &slot, match, callCallback, installCallbackHndlr,
                reinterpret_cast<void*>(callbackContext.get()));
        }
        else
        {
            ret = sd_bus_add_match(
                bus.get(), &slot, match, callCallback,
                reinterpret_cast<void*>(callbackContext.get()));
        }
        if (ret < 0)
        {
            throw sdbusplus::exception::SdBusError(-EIO,
                                                   "ERROR in add match rule");
        }

        _slot = decltype(_slot){slot};
    }
    match(sdbusplus::bus::bus& bus, const std::string& _match,
          callback_t callback, bool asyncCall = false,
          callback_t installCallback = nullptr) :
        match(bus, _match.c_str(), callback, asyncCall, installCallback)
    {
    }

  private:
    slot::slot _slot;
    std::unique_ptr<contextPair_t> callbackContext;

    static int installCallbackHndlr(sd_bus_message* m, void* context,
                                    sd_bus_error* e)
    {
        auto pairPtr = static_cast<contextPair_t*>(context);
        if (pairPtr->second)
        {
            callback_t* c = pairPtr->second.get();
            message::message message{m};
            (*c)(message);
        }
        return 0;
    }

    static int callCallback(sd_bus_message* m, void* context, sd_bus_error* e)
    {
        auto pairPtr = static_cast<contextPair_t*>(context);
        callback_t* c = pairPtr->first.get();
        message::message message{m};

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

inline auto signal()
{
    return "type='signal',"s;
}
inline auto method()
{
    return "type='method',"s;
}
inline auto method_return()
{
    return "type='method_return',"s;
}
inline auto error()
{
    return "type='error',"s;
}

} // namespace type

inline auto sender(const std::string& s)
{
    return "sender='"s + s + "',";
}
inline auto interface(const std::string& s)
{
    return "interface='"s + s + "',";
}
inline auto member(const std::string& s)
{
    return "member='"s + s + "',";
}
inline auto path(const std::string& s)
{
    return "path='"s + s + "',";
}
inline auto path_namespace(const std::string& s)
{
    return "path_namespace='"s + s + "',";
}
inline auto destination(const std::string& s)
{
    return "destination='"s + s + "',";
}
inline auto argN(size_t n, const std::string& s)
{
    return "arg"s + std::to_string(n) + "='"s + s + "',";
}
inline auto argNpath(size_t n, const std::string& s)
{
    return "arg"s + std::to_string(n) + "path='"s + s + "',";
}
inline auto arg0namespace(const std::string& s)
{
    return "arg0namespace='"s + s + "',";
}
inline auto eavesdrop()
{
    return "eavesdrop='true',"s;
}

inline auto nameOwnerChanged()
{
    return "type='signal',"
           "sender='org.freedesktop.DBus',"
           "member='NameOwnerChanged',"s;
}

inline auto interfacesAdded()
{
    return "type='signal',"
           "interface='org.freedesktop.DBus.ObjectManager',"
           "member='InterfacesAdded',"s;
}

inline auto interfacesRemoved()
{
    return "type='signal',"
           "interface='org.freedesktop.DBus.ObjectManager',"
           "member='InterfacesRemoved',"s;
}

inline auto interfacesAdded(const std::string& p)
{
    return interfacesAdded() + path(p);
}

inline auto interfacesRemoved(const std::string& p)
{
    return interfacesRemoved() + path(p);
}

inline auto propertiesChanged(const std::string& p, const std::string& i)
{
    return type::signal() + path(p) + member("PropertiesChanged"s) +
           interface("org.freedesktop.DBus.Properties"s) + argN(0, i);
}

/**
 * @brief Constructs a NameOwnerChanged match string for a service name
 *
 * @param[in] s - Service name
 *
 * @return NameOwnerChanged match string for a service name
 */
inline auto nameOwnerChanged(const std::string& s)
{
    return nameOwnerChanged() + argN(0, s);
}

} // namespace rules
} // namespace match

using match_t = match::match;

} // namespace bus
} // namespace sdbusplus
