#pragma once

#include <sdbusplus/asio/connection.hpp>

namespace sdbusplus::asio
{

template <typename OnError, typename OnSuccess>
inline void getAllProperties(sdbusplus::asio::connection& bus,
                             const std::string& service,
                             const std::string& path,
                             const std::string& interface, OnError&& onError,
                             OnSuccess&& onSuccess)
{
    using FunctionTuple = boost::callable_traits::args_t<OnSuccess>;
    using FunctionTupleType =
        typename sdbusplus::utility::decay_tuple<FunctionTuple>::type;

    bus.async_method_call(
        [onError = std::move(onError), onSuccess = std::move(onSuccess)](
            boost::system::error_code ec,
            std::tuple_element_t<0, FunctionTupleType>& ret) {
            if (ec)
            {
                onError(ec);
                return;
            }

            onSuccess(ret);
        },
        service, path, "org.freedesktop.DBus.Properties", "GetAll", interface);
}

template <typename T, typename OnError, typename OnSuccess>
inline void getProperty(sdbusplus::asio::connection& bus,
                        const std::string& service, const std::string& path,
                        const std::string& interface,
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
