#pragma once

#include <sdbusplus/asio/connection.hpp>

namespace sdbusplus::asio
{

template <typename T, typename OnError, typename OnSuccess>
inline void setProperty(sdbusplus::asio::connection& bus,
                        const std::string& service, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName, T&& propertyValue,
                        OnError&& onError, OnSuccess&& onSuccess)
{
    bus.async_method_call(
        [onError = std::move(onError),
         onSuccess = std::move(onSuccess)](boost::system::error_code ec) {
            if (ec)
            {
                onError(ec);
                return;
            }

            onSuccess();
        },
        service, path, "org.freedesktop.DBus.Properties", "Set", interface,
        propertyName, std::variant<T>(std::forward<T>(propertyValue)));
}

} // namespace sdbusplus::asio
