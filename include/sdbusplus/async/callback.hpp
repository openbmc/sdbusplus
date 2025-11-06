#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/message.hpp>

#include <type_traits>

namespace sdbusplus::async
{

namespace callback_ns
{

template <typename Fn>
concept takes_msg_handler =
    std::is_invocable_r_v<int, Fn, sd_bus_message_handler_t, void*>;

template <takes_msg_handler Init>
struct callback_sender;

} // namespace callback_ns

/** Create a sd_bus-callback Sender.
 *
 *  In the sd_bus library there are many functions named `*_async` that take
 *  a `sd_bus_message_handler_t` as the callback.  This function turns them
 *  into a Sender.
 *
 *  The `Init` function is a simple indirect so that the library sd_bus
 *  function can be called, but with the callback handler (and data) placed
 *  in the right call positions.
 *
 *  For example, `sd_bus_call_async` could be turned into a Sender with a
 *  call to this and a small lambda such as:
 *  ```
 *      callback([bus = get_busp(ctx),
 *                msg = std::move(msg)](auto cb, auto data) {
 *          return sd_bus_call_async(bus, nullptr, msg.get(), cb, data, 0);
 *      })
 *  ```
 *
 *  @param[in] i - A function which calls the underlying sd_bus library
 *                 function.
 *
 *  @returns A Sender which completes when sd-bus calls the callback and yields
 *           a `sdbusplus::message_t`.
 */
template <callback_ns::takes_msg_handler Init>
auto callback(Init i)
{
    return callback_ns::callback_sender<Init>(std::move(i));
}

namespace callback_ns
{

/** The operation which handles the Sender completion. */
template <takes_msg_handler Init, execution::receiver R>
struct callback_operation
{
    callback_operation() = delete;
    callback_operation(callback_operation&&) = delete;

    callback_operation(Init&& init, R&& r) :
        init(std::move(init)), receiver(std::move(r))
    {}

    // Handle the call from sd-bus by ensuring there were no errors
    // and setting the completion value to the resulting message.
    static int handler(sd_bus_message* m, void* cb, sd_bus_error* e) noexcept
    {
        callback_operation& self = *static_cast<callback_operation*>(cb);
        try
        {
            // Check 'e' for error.
            if ((nullptr != e) && (sd_bus_error_is_set(e)))
            {
                throw exception::SdBusError(e, "callback");
            }

            message_t msg{m};

            // Check the message response for error.
            if (msg.is_method_error())
            {
                auto err = *msg.get_error();
                throw exception::SdBusError(&err, "method");
            }

            execution::set_value(std::move(self.receiver), std::move(msg));
        }
        catch (...)
        {
            execution::set_error(std::move(self.receiver),
                                 std::current_exception());
        }
        return 0;
    }

    // Call the init function upon Sender start.
    void start() noexcept
    {
        try
        {
            auto rc = init(handler, this);
            if (rc < 0)
            {
                throw exception::SdBusError(-rc, __PRETTY_FUNCTION__);
            }
        }
        catch (...)
        {
            execution::set_error(std::move(receiver), std::current_exception());
        }
    }

  private:
    Init init;
    R receiver;
};

/** The Sender for a callback.
 *
 *  The basically just holds the Init function until the Sender is connected
 *  to (co_awaited on for co-routines), when it is turned into a pending
 *  operation.
 */
template <takes_msg_handler Init>
struct callback_sender
{
    using sender_concept = execution::sender_t;

    explicit callback_sender(Init init) : init(std::move(init)) {};

    // This Sender yields a message_t.
    template <typename Self, class... Env>
    static constexpr auto get_completion_signatures(Self&&, Env&&...)
        -> execution::completion_signatures<execution::set_value_t(message_t),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    auto connect(R r) -> callback_operation<Init, R>
    {
        return {std::move(init), std::move(r)};
    }

  private:
    Init init;
};

} // namespace callback_ns

} // namespace sdbusplus::async
