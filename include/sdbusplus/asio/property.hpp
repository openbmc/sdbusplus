#pragma once

#include <boost/type_traits.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/utility/type_traits.hpp>

namespace sdbusplus::asio
{

template <typename VariantType>
inline void getAllProperties(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface,
    std::function<void(
        const boost::system::error_code&,
        const std::vector<std::pair<std::string, VariantType>>&)>&& handler)
{
    static_assert(std::is_same_v<VariantType, std::decay_t<VariantType>>);

    bus.async_method_call(
        [handler = std::move(handler)](
            const boost::system::error_code& ec,
            const std::vector<std::pair<std::string, VariantType>>&
                data) mutable { handler(ec, data); },
        service, path, "org.freedesktop.DBus.Properties", "GetAll", interface);
}

template <typename Handler>
inline void getAllProperties(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface, Handler&& handler)
{
    using arg1_type =
        std::tuple_element_t<1, boost::callable_traits::args_t<Handler>>;
    using arg1_pair_type = typename std::decay_t<arg1_type>::value_type;
    using arg1_value_type = typename arg1_pair_type::second_type;
    getAllProperties<arg1_value_type>(bus, service, path, interface,
                                      std::forward<Handler>(handler));
}

template <typename PropertyType>
inline void getProperty(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface,
    const std::string& propertyName,
    std::function<void(boost::system::error_code, PropertyType)>&& handler)
{
    static_assert(std::is_same_v<PropertyType, std::decay_t<PropertyType>>);

    bus.async_method_call(
        [handler = std::move(handler)](
            const boost::system::error_code& ec,
            std::variant<std::monostate, PropertyType>& ret) mutable {
            if (ec)
            {
                handler(ec, {});
                return;
            }

            if (PropertyType* value = std::get_if<PropertyType>(&ret))
            {
                handler(ec, std::move(*value));
                return;
            }

            handler(boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument),
                    {});
        },
        service, path, "org.freedesktop.DBus.Properties", "Get", interface,
        propertyName);
}

/* This method has been deprecated, and will be removed in a future revision.
 * Use the getProperty overload above to make equivalent calls
 */
template <typename T, typename OnError, typename OnSuccess>
[[deprecated]] inline void getProperty(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface,
    const std::string& propertyName, OnError&& onError, OnSuccess&& onSuccess)
{
    bus.async_method_call(
        [onError = std::move(onError), onSuccess = std::move(onSuccess)](
            boost::system::error_code ec,
            std::variant<std::monostate, T>& ret) {
            if (ec)
            {
                onError(ec);
                return;
            }

            if (T* value = std::get_if<T>(&ret))
            {
                onSuccess(*value);
                return;
            }

            onError(boost::system::errc::make_error_code(
                boost::system::errc::invalid_argument));
        },
        service, path, "org.freedesktop.DBus.Properties", "Get", interface,
        propertyName);
}

template <typename PropertyType, typename Handler>
inline void setProperty(sdbusplus::asio::connection& bus,
                        const std::string& service, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName,
                        PropertyType&& propertyValue, Handler&& handler)
{
    bus.async_method_call(
        std::forward<Handler>(handler), service, path,
        "org.freedesktop.DBus.Properties", "Set", interface, propertyName,
        std::variant<std::decay_t<PropertyType>>(
            std::forward<PropertyType>(propertyValue)));
}

} // namespace sdbusplus::asio
