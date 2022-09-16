#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/event.hpp>

namespace sdbusplus::async
{

/** sleep_for Sender
 *
 *  This Sender performs the equivalent of `std::this_thread::sleep_for`,
 *  in an async context.
 *
 *  @param[in] ctx The async context.
 *  @param[in] time The length of time to delay.
 *
 *  @return A sender which completes after time.
 */
template <typename Rep, typename Period>
auto sleep_for(context& ctx, std::chrono::duration<Rep, Period> time);

namespace timer_ns
{

/* The sleep completion event.
 *
 * On start, creates the sd-event timer.
 * On callback, completes the Receiver.
 */
template <execution::receiver R>
struct sleep_operation : public details::context_friend
{
    sleep_operation() = delete;
    sleep_operation(sleep_operation&&) = delete;

    sleep_operation(context& ctx, event_t::time_resolution time, R&& r) :
        ctx(ctx), time(time), receiver(std::move(r))
    {}

    static int handler(sd_event_source*, uint64_t, void* data) noexcept
    {
        auto self = static_cast<sleep_operation<R>*>(data);
        execution::set_value(std::move(self->receiver));

        return 0;
    }

    friend auto tag_invoke(execution::start_t, sleep_operation& self) noexcept
    {
        try
        {
            self.source = self.event_loop().add_oneshot_timer(handler, &self,
                                                              self.time);
        }
        catch (...)
        {
            execution::set_error(std::move(self.receiver),
                                 std::current_exception());
        }
    }

  private:
    event_t& event_loop()
    {
        return get_event_loop(ctx);
    }

    context& ctx;
    event_t::time_resolution time;
    event_source_t source;
    R receiver;
};

/** The delay Sender.
 *
 *  On connect, instantiates the completion event.
 */
struct sleep_sender : public details::context_friend
{
    sleep_sender() = delete;

    sleep_sender(context& ctx, event_t::time_resolution time) noexcept :
        ctx(ctx), time(time)
    {}

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const sleep_sender&, auto)
        -> execution::completion_signatures<execution::set_value_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, sleep_sender&& self, R r)
        -> sleep_operation<R>
    {
        return {self.ctx, self.time, std::move(r)};
    }

    static auto sleep_for(context& ctx, event_t::time_resolution time)
    {
        // Run the delay sender and then switch back to the worker thread.
        // The delay completion happens from the sd-event handler, which is
        // ran on the 'caller' thread.

        return execution::when_all(sleep_sender(ctx, time),
                                   execution::schedule(get_scheduler(ctx)));
    }

  private:
    context& ctx;
    event_t::time_resolution time;
};

} // namespace timer_ns

template <typename Rep, typename Period>
auto sleep_for(context& ctx, std::chrono::duration<Rep, Period> time)
{
    return timer_ns::sleep_sender::sleep_for(
        ctx, std::chrono::duration_cast<event_t::time_resolution>(time));
}
} // namespace sdbusplus::async
