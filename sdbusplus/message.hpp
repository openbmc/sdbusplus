#pragma once

#include <memory>
#include <systemd/sd-bus.h>
#include <sdbusplus/message/append.hpp>

namespace sdbusplus
{

    // Forward declare sdbusplus::bus::bus for 'friend'ship.
namespace bus { struct bus; };

namespace message
{

using msgp_t = sd_bus_message*;
class message;

namespace details
{

/** @brief unique_ptr functor to release a msg reference. */
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
struct message
{
        /* Define all of the basic class operations:
         *     Not allowed:
         *         - Default constructor to avoid nullptrs.
         *         - Copy operations due to internal unique_ptr.
         *     Allowed:
         *         - Move operations.
         *         - Destructor.
         */
    message() = delete;
    message(const message&) = delete;
    message& operator=(const message&) = delete;
    message(message&&) = default;
    message& operator=(message&&) = default;
    ~message() = default;

    /** @brief Conversion constructor for 'msgp_t'.
     *
     *  Takes ownership of the msg-pointer and releases it when done.
     */
    explicit message(msgp_t m) : _msg(m) {};

    /** @brief Release ownership of the stored msg-pointer. */
    msgp_t release() { return _msg.release(); };

    /** @brief Perform sd_bus_message_append, with automatic type deduction.
     *
     *  @tparam ...Args - Type of items to append to message.
     *  @param[in] args - Items to append to message.
     */
    template <typename ...Args> void append(Args&&... args)
    {
        sdbusplus::message::append(_msg.get(), std::forward<Args>(args)...);
    }

    friend struct sdbusplus::bus::bus;

    private:
        /** @brief Get a pointer to the owned 'msgp_t'. */
        msgp_t get() { return _msg.get(); };
        details::msg _msg;
};

} // namespace message

} // namespace sdbusplus
