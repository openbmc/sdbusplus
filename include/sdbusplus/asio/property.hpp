#pragma once

#include <boost/type_traits.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/utility/type_traits.hpp>

namespace sdbusplus::asio
{

template <typename T>
inline void getAllProperties(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface,
    std::function<void(const boost::system::error_code,
                       const std::vector<std::pair<std::string, T>>&)>&&
        handler)
{
    static_assert(std::is_same_v<T, std::decay_t<T>>);

    bus.async_method_call(std::move(handler), service, path,
                          "org.freedesktop.DBus.Properties", "GetAll",
                          interface);
}

template <typename T>
inline void getAllProperties(sdbusplus::asio::connection& bus,
                             const std::string& service,
                             const std::string& path,
                             const std::string& interface, T&& handler)
{
    using arg1_type =
        std::tuple_element_t<1, boost::callable_traits::args_t<T>>;
    using arg1_pair_type = std::decay_t<arg1_type>::value_type;
    using arg1_value_type = arg1_pair_type::second_type;
    getAllProperties<arg1_value_type>(bus, service, path, interface,
                                      std::forward<T>(handler));
}

template <typename T>
inline void
    getProperty(sdbusplus::asio::connection& bus, const std::string& service,
                const std::string& path, const std::string& interface,
                const std::string& propertyName,
                std::function<void(boost::system::error_code, T)>&& handler)
{
    static_assert(std::is_same_v<T, std::decay_t<T>>);

    bus.async_method_call(
        [handler = std::move(handler)](boost::system::error_code ec,
                                       std::variant<std::monostate, T>& ret) {
            if (ec)
            {
                handler(ec, {});
                return;
            }

            if (T* value = std::get_if<T>(&ret))
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
[[deprecated]] inline void
    getProperty(sdbusplus::asio::connection& bus, const std::string& service,
                const std::string& path, const std::string& interface,
                const std::string& propertyName, OnError&& onError,
                OnSuccess&& onSuccess)
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

template <typename T, typename Handler>
inline void setProperty(sdbusplus::asio::connection& bus,
                        const std::string& service, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName, T&& propertyValue,
                        Handler&& handler)
{
    bus.async_method_call(
        std::forward<Handler>(handler), service, path,
        "org.freedesktop.DBus.Properties", "Set", interface, propertyName,
        std::variant<std::decay_t<T>>(std::forward<T>(propertyValue)));
}

} // namespace sdbusplus::asio
