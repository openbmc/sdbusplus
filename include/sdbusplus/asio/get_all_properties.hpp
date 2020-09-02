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

} // namespace sdbusplus::asio
