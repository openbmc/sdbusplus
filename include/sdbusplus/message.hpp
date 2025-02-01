#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/append.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/message/read.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/slot.hpp>

#include <exception>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#endif

namespace sdbusplus
{

// Forward declare sdbusplus::bus::bus for 'friend'ship.
namespace bus
{
struct bus;
}

namespace message
{

using msgp_t = sd_bus_message*;
class message;

namespace details
{

/** @brief unique_ptr functor to release a msg reference. */
// TODO(venture): Consider using template <SdBusInterfaceType> for this so that
// it doesn't require creating a specific instance of it, unless that's ok --
struct MsgDeleter
{
    void operator()(msgp_t ptr) const
    {
        sd_bus_message_unref(ptr);
    }
};

/* @brief Alias 'msg' to a unique_ptr type for auto-release. */
using msg = std::unique_ptr<sd_bus_message, MsgDeleter>;

template <typename CbT>
int call_async_cb(sd_bus_message* m, void* userdata, sd_bus_error*) noexcept;

template <typename CbT>
void call_async_del(void* userdata) noexcept
{
    delete reinterpret_cast<CbT*>(userdata);
}

} // namespace details

/** @class message
 *  @brief Provides C++ bindings to the sd_bus_message_* class functions.
 */
class message : private sdbusplus::slot::details::slot_friend
{
    /* Define all of the basic class operations:
     *     Allowed:
     *         - Default constructor
     *         - Copy operations.
     *         - Move operations.
     *         - Destructor.
     */
  public:
    message(message&&) = default;
    message& operator=(message&&) = default;
    ~message() = default;

    message(msgp_t m, sdbusplus::SdBusInterface* intf) :
        _intf(std::move(intf)), _msg(_intf->sd_bus_message_ref(m))
    {}

    /** @brief Conversion constructor for 'msgp_t'.
     *
     *  Takes increment ref-count of the msg-pointer and release when
     *  destructed.
     */
    explicit message(msgp_t m = nullptr) : message(m, &sdbus_impl) {}

    message(msgp_t m, sdbusplus::SdBusInterface* intf, std::false_type) :
        _intf(intf), _msg(m)
    {}

    /** @brief Constructor for 'msgp_t'.
     *
     *  Takes ownership of the msg-pointer and releases it when done.
     */
    message(msgp_t m, std::false_type) : _intf(&sdbus_impl), _msg(m) {}

    /** @brief Copy constructor for 'message'.
     *
     *  Copies the message class and increments the ref on _msg
     */
    message(const message& other) :
        _intf(other._intf), _msg(sd_bus_message_ref(other._msg.get()))
    {}

    /** @brief Assignment operator for 'message'.
     *
     *  Copies the message class and increments the ref on _msg
     */
    message& operator=(const message& other)
    {
        _msg.reset(sd_bus_message_ref(other._msg.get()));
        _intf = other._intf;
        return *this;
    }

    /** @brief Release ownership of the stored msg-pointer. */
    msgp_t release()
    {
        return _msg.release();
    }

    /** @brief Check if message contains a real pointer. (non-nullptr). */
    explicit operator bool() const
    {
        return bool(_msg);
    }

    /** @brief Perform sd_bus_message_append, with automatic type deduction.
     *
     *  @tparam Args - Type of items to append to message.
     *  @param[in] args - Items to append to message.
     */
    template <typename... Args>
    void append(Args&&... args)
    {
        sdbusplus::message::append(_intf, _msg.get(),
                                   std::forward<Args>(args)...);
    }

    /** @brief Perform sd_bus_message_read, with automatic type deduction.
     *
     *  @tparam Args - Type of items to read from message.
     *  @param[out] args - Items to read from message.
     */
    template <typename... Args>
    void read(Args&&... args)
    {
        sdbusplus::message::read(_intf, _msg.get(),
                                 std::forward<Args>(args)...);
    }

    /** @brief Perform sd_bus_message_read with results returned.
     *
     *  @tparam Args - Type of items to read from the message.
     *  @return One of { void, Args, std::tuple<Args...> }.
     */
    template <typename... Args>
    auto unpack()
    {
        if constexpr (sizeof...(Args) == 0)
        {
            // No args
            return;
        }
        else if constexpr (sizeof...(Args) == 1 &&
                           std::is_void_v<
                               std::tuple_element_t<0, std::tuple<Args...>>>)
        {
            // Args is void
            return;
        }
        else if constexpr (sizeof...(Args) == 1)
        {
            std::tuple_element_t<0, std::tuple<Args...>> r{};
            read(r);
            return r;
        }
        else
        {
            std::tuple<Args...> r{};
            std::apply([this](auto&&... v) { this->read(v...); }, r);
            return r;
        }
    }

    /** @brief Get the dbus bus from the message. */
    // Forward declare.
    bus::bus get_bus() const;

    /** @brief Get the signature of a message.
     *
     *  @return A [weak] pointer to the signature of the message.
     */
    const char* get_signature() const
    {
        return _intf->sd_bus_message_get_signature(_msg.get(), true);
    }

    /** @brief Get the path of a message.
     *
     *  @return A [weak] pointer to the path of the message.
     */
    const char* get_path() const
    {
        return _intf->sd_bus_message_get_path(_msg.get());
    }

    /** @brief Get the interface of a message.
     *
     *  @return A [weak] pointer to the interface of the message.
     */
    const char* get_interface() const
    {
        return _intf->sd_bus_message_get_interface(_msg.get());
    }

    /** @brief Get the member of a message.
     *
     *  @return A [weak] pointer to the member of the message.
     */
    const char* get_member() const
    {
        return _intf->sd_bus_message_get_member(_msg.get());
    }

    /** @brief Get the destination of a message.
     *
     *  @return A [weak] pointer to the destination of the message.
     */
    const char* get_destination() const
    {
        return _intf->sd_bus_message_get_destination(_msg.get());
    }

    /** @brief Get the sender of a message.
     *
     *  @return A [weak] pointer to the sender of the message.
     */
    const char* get_sender() const
    {
        return _intf->sd_bus_message_get_sender(_msg.get());
    }

    /** @brief Check if message is a method error.
     *
     *  @return True - if message is a method error.
     */
    bool is_method_error() const
    {
        return _intf->sd_bus_message_is_method_error(_msg.get(), nullptr);
    }

    /** @brief Get the errno from the message.
     *
     *  @return The errno of the message.
     */
    int get_errno() const
    {
        return _intf->sd_bus_message_get_errno(_msg.get());
    }

    /** @brief Get the error from the message.
     *
     *  @return The error of the message.
     */
    const sd_bus_error* get_error() const
    {
        return _intf->sd_bus_message_get_error(_msg.get());
    }

    /** @brief Get the type of a message.
     *
     * @return The type of message.
     */
    auto get_type() const
    {
        uint8_t type;
        int r = _intf->sd_bus_message_get_type(_msg.get(), &type);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_get_type");
        }
        return type;
    }

    /** @brief Get the transaction cookie of a message.
     *
     * @return The transaction cookie of a message.
     */
    auto get_cookie() const
    {
        uint64_t cookie;
        int r = _intf->sd_bus_message_get_cookie(_msg.get(), &cookie);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_get_cookie");
        }
        return cookie;
    }

    /** @brief Get the reply cookie of a message.
     *
     * @return The reply cookie of a message.
     */
    auto get_reply_cookie() const
    {
        uint64_t cookie;
        int r = _intf->sd_bus_message_get_reply_cookie(_msg.get(), &cookie);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_get_reply_cookie");
        }
        return cookie;
    }

    /** @brief Check if message is a method call for an interface/method.
     *
     *  @param[in] interface - The interface to match.
     *  @param[in] method - The method to match.
     *
     *  @return True - if message is a method call for interface/method.
     */
    bool is_method_call(const char* interface, const char* method) const
    {
        return _intf->sd_bus_message_is_method_call(_msg.get(), interface,
                                                    method);
    }

    /** @brief Check if message is a signal for an interface/member.
     *
     *  @param[in] interface - The interface to match.
     *  @param[in] member - The member to match.
     */
    bool is_signal(const char* interface, const char* member) const
    {
        return _intf->sd_bus_message_is_signal(_msg.get(), interface, member);
    }

    /** @brief Create a 'method_return' type message from an existing message.
     *
     *  @return method-return message.
     */
    message new_method_return()
    {
        msgp_t reply = nullptr;
        int r = _intf->sd_bus_message_new_method_return(this->get(), &reply);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_new_method_return");
        }

        return message(reply, _intf, std::false_type());
    }

    /** @brief Create a 'method_error' type message from an existing message.
     *
     *  @param[in] e - The exception we are returning
     *  @return method-return message.
     */
    message new_method_error(const sdbusplus::exception::exception& e)
    {
        msgp_t reply = nullptr;
        sd_bus_error error = SD_BUS_ERROR_NULL;
        e.set_error(_intf, &error);
        int r =
            _intf->sd_bus_message_new_method_error(this->get(), &reply, &error);
        sd_bus_error_free(&error);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_new_method_error");
        }

        return message(reply, _intf, std::false_type());
    }

    /** @brief Create a 'method_error' type message from an existing message.
     *
     *  @param[in] error - integer error number
     *  @param[in] e - optional pointer to preformatted sd_bus_error
     *  @return method-error message.
     */
    message new_method_errno(int error, const sd_bus_error* e = nullptr)
    {
        msgp_t reply = nullptr;
        int r = _intf->sd_bus_message_new_method_errno(this->get(), &reply,
                                                       error, e);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_new_method_errno");
        }

        return message(reply, _intf, std::false_type());
    }

    /** @brief Perform a 'method-return' response call. */
    void method_return()
    {
        auto b = _intf->sd_bus_message_get_bus(this->get());
        int r = _intf->sd_bus_send(b, this->get(), nullptr);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_send");
        }
    }

    /** @brief Perform a 'signal-send' call. */
    void signal_send()
    {
        method_return();
    }

    /** @brief Perform a message call.
     *         Errors generated by this call come from underlying dbus
     *         related errors *AND* from any method call that results
     *         in a METHOD_ERROR. This means you do not need to check
     *         is_method_error() on the returned message.
     *
     *  @param[in] timeout - The timeout for the method call.
     *
     *  @return The response message.
     */
    auto call(std::optional<SdBusDuration> timeout = std::nullopt)
    {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;
        auto timeout_us = timeout ? timeout->count() : 0;
        int r = _intf->sd_bus_call(nullptr, get(), timeout_us, &error, &reply);
        if (r < 0)
        {
            throw exception::SdBusError(&error, "sd_bus_call");
        }

        return message(reply, _intf, std::false_type());
    }

    /** @brief Perform an async message call.
     *
     *  @param[in] cb - The callback to run when the response is available.
     *  @param[in] timeout - The timeout for the method call.
     *
     *  @return The slot handle that manages the lifetime of the call object.
     */
    template <typename Cb>
    [[nodiscard]] slot_t call_async(
        Cb&& cb, std::optional<SdBusDuration> timeout = std::nullopt)
    {
        sd_bus_slot* slot;
        auto timeout_us = timeout ? timeout->count() : 0;
        using CbT = std::remove_cv_t<std::remove_reference_t<Cb>>;
        int r = _intf->sd_bus_call_async(nullptr, &slot, get(),
                                         details::call_async_cb<CbT>, nullptr,
                                         timeout_us);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_call_async");
        }
        slot_t ret(slot, _intf);

        if constexpr (std::is_pointer_v<CbT>)
        {
            _intf->sd_bus_slot_set_userdata(get_slotp(ret),
                                            reinterpret_cast<void*>(cb));
        }
        else if constexpr (std::is_function_v<CbT>)
        {
            _intf->sd_bus_slot_set_userdata(get_slotp(ret),
                                            reinterpret_cast<void*>(&cb));
        }
        else
        {
            r = _intf->sd_bus_slot_set_destroy_callback(
                get_slotp(ret), details::call_async_del<CbT>);
            if (r < 0)
            {
                throw exception::SdBusError(-r,
                                            "sd_bus_slot_set_destroy_callback");
            }
            _intf->sd_bus_slot_set_userdata(get_slotp(ret),
                                            new CbT(std::forward<Cb>(cb)));
        }
        return ret;
    }

    friend struct sdbusplus::bus::bus;

    /** @brief Get a pointer to the owned 'msgp_t'.
     * This api should be used sparingly and carefully, as it opens a number of
     * possibilities for race conditions, RAII destruction issues, and runtime
     * problems when using the sd-bus c api.  Here be dragons. */
    msgp_t get()
    {
        return _msg.get();
    }

  private:
    sdbusplus::SdBusInterface* _intf;
    details::msg _msg;
};

namespace details
{

template <typename CbT>
int call_async_cb(sd_bus_message* m, void* userdata, sd_bus_error*) noexcept
{
    try
    {
        if constexpr (std::is_pointer_v<CbT>)
        {
            (*reinterpret_cast<CbT>(userdata))(message(m));
        }
        else
        {
            (*reinterpret_cast<CbT*>(userdata))(message(m));
        }
    }
    catch (...)
    {
        std::terminate();
    }
    return 1;
}

} // namespace details

} // namespace message

using message_t = message::message;

} // namespace sdbusplus

#ifdef __clang__
#pragma clang diagnostic pop
#endif
