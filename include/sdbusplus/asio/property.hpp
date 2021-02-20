#pragma once

#include <sdbusplus/asio/connection.hpp>

namespace sdbusplus::asio
{

template <typename Handler>
inline void getAllProperties(sdbusplus::asio::connection& bus,
                             const std::string& service,
                             const std::string& path,
                             const std::string& interface, Handler&& handler)
{
    using FunctionTuple = boost::callable_traits::args_t<Handler>;
    using FunctionTupleType =
        typename sdbusplus::utility::decay_tuple<FunctionTuple>::type;

    bus.async_method_call(
        [handler = std::move(handler)](
            boost::system::error_code ec,
            std::tuple_element_t<1, FunctionTupleType>& ret) {
            handler(ec, ret);
        },
        service, path, "org.freedesktop.DBus.Properties", "GetAll", interface);
}

template <typename T, typename Handler>
inline void getProperty(sdbusplus::asio::connection& bus,
                        const std::string& service, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName, Handler&& handler)
{
    bus.async_method_call(
        [Handler = std::move(handler)](boost::system::error_code ec,
                                       std::variant<std::monostate, T>& ret) {
            if (ec)
            {
                handler(ec, {});
                return;
            }

            if (T* value = std::get_if<T>(&ret))
            {
                handler(ec, *value);
                return;
            }

            handler(boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument),
                    ret);
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
        [handler = std::move(handler)](boost::system::error_code ec) {
            handler(ec);
        },
        service, path, "org.freedesktop.DBus.Properties", "Set", interface,
        propertyName,
        std::variant<std::decay_t<T>>(std::forward<T>(propertyValue)));
}

} // namespace sdbusplus::asio
