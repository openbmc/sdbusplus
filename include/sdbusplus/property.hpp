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
        auto propertyVar =
            msg.unpack<std::variant<std::monostate, PropertyType>>();
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

/**
 * Property subscription object returned from a call to subscribeToProperty().
 * When this is destroyed (or cleared by assigning {}), the subscription is
 * cancelled.
 */
struct PropertyMatch
{
    std::optional<sdbusplus::bus::match_t> propChangedMatch;
    std::optional<sdbusplus::bus::match_t> intfAddedMatch;
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

    // The user's handler is moved into this common handler functor. This common
    // object is shared via shared_ptr by the two match callbacks.
    struct CommonPropHandler
    {
        std::string name;
        PropertyHandler<PropertyType> handler;
        void operator()(const ChangedPropertiesType& changedProps)
        {
            for (const auto& [changedPropName, propVar] : changedProps)
            {
                if (changedPropName != name)
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
        }
    };
    auto commonHandler = std::make_shared<CommonPropHandler>(
        CommonPropHandler{propertyName, std::move(handler)});

    namespace rules = bus::match::rules;
    sdbusplus::bus::match_t propMatch(
        bus,
        rules::sender(service) + rules::propertiesChanged(object, interface),
        [commonHandler](sdbusplus::message_t& msg) {
        // ignore first param (interface name), it has to be correct
        auto [_,
              changedProps] = msg.unpack<std::string, ChangedPropertiesType>();
        (*commonHandler)(changedProps);
    });

    sdbusplus::bus::match_t intfMatch(
        bus, rules::sender(service) + rules::interfacesAdded(),
        [object, interface, commonHandler](sdbusplus::message_t& msg) {
        auto changedObject = msg.unpack<sdbusplus::message::object_path>();
        if (changedObject != object)
        {
            return;
        }

        auto changedInterfaces = msg.unpack<
            std::vector<std::pair<std::string, ChangedPropertiesType>>>();

        for (const auto& [changedInterface, changedProps] : changedInterfaces)
        {
            if (changedInterface != interface)
            {
                continue;
            }

            (*commonHandler)(changedProps);
            break;
        }
    });

    return {std::move(propMatch), std::move(intfMatch)};
}

} // namespace sdbusplus
