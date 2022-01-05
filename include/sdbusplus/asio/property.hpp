#pragma once

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/utility/type_traits.hpp>

namespace sdbusplus::asio
{

template <typename T>
inline void getAllProperties(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface,
    std::function<void(const boost::system::error_code, T&)>&& handler)
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
    getAllProperties<typename utility::functor_traits<T>::arg_t<1>>(
        bus, service, path, interface, std::forward<T>(handler));
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
