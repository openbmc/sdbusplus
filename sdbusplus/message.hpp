#pragma once

#include <systemd/sd-bus.h>

#include <cassert>
#include <memory>
#include <sdbusplus/message/append.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/message/read.hpp>
#include <sdbusplus/sdbus.hpp>
#include <type_traits>

namespace sdbusplus
{

// Forward declare sdbusplus::bus::bus for 'friend'ship.
namespace bus
{
struct bus;
};

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

} // namespace details

/** @class message
 *  @brief Provides C++ bindings to the sd_bus_message_* class functions.
 */
class message
{
    /* Define all of the basic class operations:
     *     Allowed:
     *         - Default constructor (requires checks for _msg == nullptr)
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
    {
    }

    /** @brief Conversion constructor for 'msgp_t'.
     *
     *  Takes increment ref-count of the msg-pointer and release when
     *  destructed.
     */
    explicit message(msgp_t m = nullptr) : message(m, &sdbus_impl)
    {
    }

    message(msgp_t m, sdbusplus::SdBusInterface* intf, std::false_type) :
        _intf(intf), _msg(m)
    {
    }

    /** @brief Constructor for 'msgp_t'.
     *
     *  Takes ownership of the msg-pointer and releases it when done.
     */
    message(msgp_t m, std::false_type) : _intf(&sdbus_impl), _msg(m)
    {
    }

    /** @brief Copy constructor for 'message'.
     *
     *  Copies the message class and increments the ref on _msg
     */
    message(const message& other) :
        _intf(other._intf), _msg(sd_bus_message_ref(other._msg.get()))
    {
    }

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
     *  @tparam ...Args - Type of items to append to message.
     *  @param[in] args - Items to append to message.
     */
    template <typename... Args>
    void append(Args&&... args)
    {
        assert(_msg);
        sdbusplus::message::append(_intf, _msg.get(),
                                   std::forward<Args>(args)...);
    }

    /** @brief Perform sd_bus_message_read, with automatic type deduction.
     *
     *  @tparam ...Args - Type of items to read from message.
     *  @param[out] args - Items to read from message.
     */
    template <typename... Args>
    void read(Args&&... args)
    {
        assert(_msg);
        sdbusplus::message::read(_intf, _msg.get(),
                                 std::forward<Args>(args)...);
    }

    /** @brief Get the dbus bus from the message. */
    // Forward declare.
    auto get_bus();

    /** @brief Get the signature of a message.
     *
     *  @return A [weak] pointer to the signature of the message.
     */
    const char* get_signature()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_signature(_msg.get(), true);
    }

    /** @brief Get the path of a message.
     *
     *  @return A [weak] pointer to the path of the message.
     */
    const char* get_path()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_path(_msg.get());
    }

    /** @brief Get the interface of a message.
     *
     *  @return A [weak] pointer to the interface of the message.
     */
    const char* get_interface()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_interface(_msg.get());
    }

    /** @brief Get the member of a message.
     *
     *  @return A [weak] pointer to the member of the message.
     */
    const char* get_member()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_member(_msg.get());
    }

    /** @brief Get the destination of a message.
     *
     *  @return A [weak] pointer to the destination of the message.
     */
    const char* get_destination()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_destination(_msg.get());
    }

    /** @brief Get the sender of a message.
     *
     *  @return A [weak] pointer to the sender of the message.
     */
    const char* get_sender()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_sender(_msg.get());
    }

    /** @brief Check if message is a method error.
     *
     *  @return True - if message is a method error.
     */
    bool is_method_error()
    {
        assert(_msg);
        return _intf->sd_bus_message_is_method_error(_msg.get(), nullptr);
    }

    /** @brief Get the errno from the message.
     *
     *  @return The errno of the message.
     */
    int get_errno()
    {
        assert(_msg);
        return _intf->sd_bus_message_get_errno(_msg.get());
    }

    /** @brief Get the transaction cookie of a message.
     *
     * @return The transaction cookie of a message.
     */
    auto get_cookie()
    {
        assert(_msg);
        uint64_t cookie;
        _intf->sd_bus_message_get_cookie(_msg.get(), &cookie);
        return cookie;
    }

    /** @brief Check if message is a method call for an interface/method.
     *
     *  @param[in] interface - The interface to match.
     *  @param[in] method - The method to match.
     *
     *  @return True - if message is a method call for interface/method.
     */
    bool is_method_call(const char* interface, const char* method)
    {
        assert(_msg);
        return _intf->sd_bus_message_is_method_call(_msg.get(), interface,
                                                    method);
    }

    /** @brief Check if message is a signal for an interface/member.
     *
     *  @param[in] interface - The interface to match.
     *  @param[in] member - The member to match.
     */
    bool is_signal(const char* interface, const char* member)
    {
        assert(_msg);
        return _intf->sd_bus_message_is_signal(_msg.get(), interface, member);
    }

    /** @brief Create a 'method_return' type message from an existing message.
     *
     *  @return method-return message.
     */
    message new_method_return()
    {
        msgp_t reply = nullptr;
        _intf->sd_bus_message_new_method_return(this->get(), &reply);

        return message(reply, std::false_type());
    }

    /** @brief Perform a 'method-return' response call. */
    void method_return()
    {
        auto b = _intf->sd_bus_message_get_bus(this->get());
        _intf->sd_bus_send(b, this->get(), nullptr);
    }

    /** @brief Perform a 'signal-send' call. */
    void signal_send()
    {
        method_return();
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

} // namespace message

} // namespace sdbusplus
