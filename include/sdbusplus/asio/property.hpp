#pragma once

#include <sdbusplus/asio/connection.hpp>

namespace sdbusplus::asio
{
namespace details
{

template <class T>
std::function<void(boost::system::error_code,
                   const std::variant<std::monostate, T>&)>
    makeTypeErasedCallback(
        std::function<void(boost::system::error_code, const T&)>&& handler)
{
    return [handler = std::move(handler)](
               boost::system::error_code ec,
               const std::variant<std::monostate, T>& arg) {
        if (ec)
        {
            handler(ec, {});
            return;
        }

        if (const T* value = std::get_if<T>(&arg))
        {
            handler(ec, std::move(*value));
            return;
        }

        handler(boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument),
                {});
    };
}

} // namespace details

template <typename Handler>
inline void getAllProperties(sdbusplus::asio::connection& bus,
                             const std::string& service,
                             const std::string& path,
                             const std::string& interface, Handler&& handler)
{
    bus.async_method_call(std::forward<Handler>(handler), service, path,
                          "org.freedesktop.DBus.Properties", "GetAll",
                          interface);
}

template <typename T>
inline void getProperty(
    sdbusplus::asio::connection& bus, const std::string& service,
    const std::string& path, const std::string& interface,
    const std::string& propertyName,
    std::function<void(boost::system::error_code, const T&)>&& handler)
{
    bus.async_method_call(
        details::makeTypeErasedCallback<T>(std::move(handler)), service, path,
        "org.freedesktop.DBus.Properties", "Get", interface, propertyName);
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
