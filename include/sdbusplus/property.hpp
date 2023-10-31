#pragma once

#include "sdbusplus/message/native_types.hpp"

#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/utility/dedup_variant.hpp>

#include <system_error>

namespace sdbusplus
{

/**
 * Handler to receive a current/updated D-Bus property value.

 * The first argument is a std::error_code to communicate any D-Bus errors. Any
 * underlying sd_bus error is converted into a generic-category std::error_code.
 * Other errors may result from unexpected/incorrect types encountered during
 * parsing of an otherwise valid D-Bus reply message.
 *
 * The second argument is the actual property value. The value will be
 * default initialized if the error code is non-zero.
 */
template <typename PropertyType>
using PropertyHandler =
    std::function<void(std::error_code, const PropertyType&)>;

/**
 * Get a D-Bus property asynchronously (event loop agnostic).
 */
template <typename PropertyType>
void getProperty(sdbusplus::bus_t& bus, const std::string& service,
                 const std::string& object, const std::string& interface,
                 const std::string& propertyName,
                 PropertyHandler<PropertyType> handler)
{
    static_assert(std::is_same_v<PropertyType, std::decay_t<PropertyType>>);

    auto msg = bus.new_method_call(service.c_str(), object.c_str(),
                                   "org.freedesktop.DBus.Properties", "Get");

    msg.append(interface);
    msg.append(propertyName);

    // Automatic lifetime management of the slot_t. It's only safe because
    // sd_bus itself refs the slot while executing the callback (which is not
    // clear from the documentation but we can see at
    // https://github.com/systemd/systemd/blob/f456fa23b7e6a84d520dd63cdef745d4db7848bb/src/libsystemd/sd-bus/sd-bus.c#L2803C42-L2803C42).
    auto slotPtr = std::make_shared<slot_t>();
    *slotPtr = msg.call_async([slotPtr, handler = std::move(handler)](
                                  sdbusplus::message_t msg) mutable {
        if (msg.is_method_error())
        {
            handler(std::error_code(msg.get_errno(), std::generic_category()),
                    PropertyType());
            return;
        }
        std::variant<std::monostate, PropertyType> propertyVar;
        msg.read(propertyVar);
        const auto* propertyPtr = std::get_if<PropertyType>(&propertyVar);
        if (propertyPtr != nullptr)
        {
            handler(std::error_code(), *propertyPtr);
        }
        else
        {
            // Type mismatch
            handler(std::make_error_code(std::errc::bad_message),
                    PropertyType());
        }
        slotPtr.reset();
    });
}

struct PropertyMatch
{
    std::array<std::unique_ptr<sdbusplus::bus::match_t>, 2> matches = {};
};

/**
 * Register a handler to be called whenever the given property is changed. This
 * uses getProperty() to immediately get the current property value, and
 * registers matches to handle subsequent PropertiesChanged and InterfacesAdded
 * signals.
 *
 * @return  Match object wrapper. Keep this returned object valid for as long as
 *          you wish to continue getting property updates.
 */
template <typename PropertyType>
[[nodiscard]] PropertyMatch
    subscribeToProperty(sdbusplus::bus_t& bus, const std::string& service,
                        const std::string& object, const std::string& interface,
                        const std::string& propertyName,
                        PropertyHandler<PropertyType> handler)
{
    getProperty<PropertyType>(bus, service, object, interface, propertyName,
                              handler);

    using ChangedPropertiesType = std::vector<
        std::pair<std::string, std::variant<std::monostate, PropertyType>>>;

    auto commonPropHandler = [propertyName, handler = std::move(handler)](
                                 const ChangedPropertiesType& changedProps) {
        for (const auto& [changedPropName, propVar] : changedProps)
        {
            if (changedPropName != propertyName)
            {
                continue;
            }
            const auto* propValPtr = std::get_if<PropertyType>(&propVar);

            if (propValPtr != nullptr)
            {
                handler(std::error_code(), *propValPtr);
            }
            else
            {
                // Type mismatch
                handler(std::make_error_code(std::errc::bad_message),
                        PropertyType());
            }
            break;
        }
    };

    namespace rules = bus::match::rules;
    auto propMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus,
        rules::sender(service) + rules::propertiesChanged(object, interface),
        [commonPropHandler](sdbusplus::message_t& msg) {
        ChangedPropertiesType changedProps;
        // ignore first param (interface name), it has to be correct
        msg.read(std::string(), changedProps);
        commonPropHandler(changedProps);
    });

    auto intfMatch = std::make_unique<sdbusplus::bus::match_t>(
        bus, rules::sender(service) + rules::interfacesAdded(),
        [object, interface, commonPropHandler = std::move(commonPropHandler)](
            sdbusplus::message_t& msg) {
        sdbusplus::message::object_path changedObject;
        msg.read(changedObject);
        if (changedObject != object)
        {
            return;
        }

        std::vector<std::pair<std::string, ChangedPropertiesType>>
            changedInterfaces;
        msg.read(changedInterfaces);

        for (const auto& [changedInterface, changedProps] : changedInterfaces)
        {
            if (changedInterface != interface)
            {
                continue;
            }

            commonPropHandler(changedProps);
            break;
        }
    });

    return {std::move(propMatch), std::move(intfMatch)};
}

} // namespace sdbusplus
