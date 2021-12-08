#pragma once

#include <sdbusplus/asio/connection.hpp>

namespace sdbusplus::asio
{
namespace details
{

template <class T>
struct TypeErasedCallbackInterface
{
    virtual ~TypeErasedCallbackInterface() = default;

    virtual void operator()(boost::system::error_code, T) const = 0;
};

template <class T, class Handler>
struct TypeErasedCallbackImpl : public TypeErasedCallbackInterface<T>
{
    TypeErasedCallbackImpl(Handler&& handler) :
        handler(std::forward<Handler>(handler))
    {}

    virtual void operator()(boost::system::error_code ec,
                            T value) const override
    {
        handler(ec, std::move(value));
    }

  private:
    Handler handler;
};

template <class T>
struct TypeErasedCallback
{
    TypeErasedCallback(
        std::unique_ptr<TypeErasedCallbackInterface<T>> handler) :
        handler(std::move(handler))
    {}

    void operator()(boost::system::error_code ec,
                    std::variant<std::monostate, T>& arg) const
    {
        if (ec)
        {
            (*handler)(ec, {});
            return;
        }

        if (T* value = std::get_if<T>(&arg))
        {
            (*handler)(ec, std::move(*value));
            return;
        }

        (*handler)(boost::system::errc::make_error_code(
                       boost::system::errc::invalid_argument),
                   {});
    }

  private:
    std::unique_ptr<TypeErasedCallbackInterface<T>> handler;
};

template <class T, class Handler>
TypeErasedCallback<T> makeTypeErasedCallback(Handler&& handler)
{
    auto typeErasedCallbackImpl =
        std::make_unique<TypeErasedCallbackImpl<T, Handler>>(
            std::forward<Handler>(handler));
    return {std::move(typeErasedCallbackImpl)};
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

template <typename T, typename Handler>
inline void getProperty(sdbusplus::asio::connection& bus,
                        const std::string& service, const std::string& path,
                        const std::string& interface,
                        const std::string& propertyName, Handler&& handler)
{
    bus.async_method_call(
        details::makeTypeErasedCallback<T>(std::forward<Handler>(handler)),
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
