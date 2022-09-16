#include <systemd/sd-bus.h>

#include <sdbusplus/async/context.hpp>

#include <chrono>

namespace sdbusplus::async
{

context::context(bus_t&& b) : bus(std::move(b))
{
    dbus_source =
        event_loop.add_io(bus.get_fd(), EPOLLIN, dbus_event_handle, this);
}

namespace details
{

/* The sd_bus_wait/process completion event.
 *
 *  The wait/process handshake is modelled as a Sender with the the worker
 *  task `co_await`ing Senders over and over.  This class is the completion
 *  handling for the Sender (to get it back to the Receiver, ie. the worker).
 */
struct wait_process_completion : bus::details::bus_friend
{
    explicit wait_process_completion(context& ctx) : ctx(ctx) {}
    virtual ~wait_process_completion() = default;

    // Called by the `caller` to indicate the Sender is completed.
    virtual void complete() noexcept = 0;
    // Called by the `caller` to indicate the Sender should be stopped.
    virtual void stop() noexcept = 0;

    // Arm the completion event.
    void arm() noexcept;

    // Data to share with the worker.
    context& ctx;
    std::chrono::microseconds timeout{};

    static task<> loop(context& ctx);
    static void wait_once(context& ctx);
};

/* The completion template based on receiver type.
 *
 * The type of the receivers (typically the co_awaiter) is only known by
 * a template, so we need a sub-class of completion to hold the receiver.
 */
template <execution::receiver R>
struct wait_process_operation : public wait_process_completion
{
    wait_process_operation(context& ctx, R r) :
        wait_process_completion(ctx), receiver(std::move(r))
    {}

    wait_process_operation(wait_process_operation&&) = delete;

    void complete() noexcept override final
    {
        execution::set_value(std::move(this->receiver));
    }

    void stop() noexcept override final
    {
        execution::set_stopped(std::move(this->receiver));
    }

    friend void tag_invoke(execution::start_t,
                           wait_process_operation& self) noexcept
    {
        self.arm();
    }

    R receiver;
};

/* The sender for the wait/process event. */
struct wait_process_sender
{
    explicit wait_process_sender(context& ctx) : ctx(ctx){};

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const wait_process_sender&, auto)
        -> execution::completion_signatures<execution::set_value_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, wait_process_sender&& self,
                           R r) -> wait_process_operation<R>
    {
        // Create the completion for the wait.
        return {self.ctx, std::move(r)};
    }

  private:
    context& ctx;
};

task<> wait_process_completion::loop(context& ctx)
{
    while (!ctx.stop_requested())
    {
        // Handle the next sdbus event.
        co_await wait_process_sender(ctx);

        // Completion likely happened on the context 'caller' thread, so
        // we need to switch to the worker thread.
        co_await execution::schedule(ctx.loop.get_scheduler());
    }
}

} // namespace details

context::~context() noexcept(false)
{
    if (worker_thread.joinable())
    {
        throw std::logic_error(
            "sdbusplus::async::context destructed without completion.");
    }
}

bool context::request_stop() noexcept
{
    auto first_stop = stop.request_stop();

    if (first_stop)
    {
        caller_wait.notify_one();
        event_loop.break_run();
    }

    return first_stop;
}

void context::caller_run(task<> startup)
{
    // Start up the worker thread.
    worker_thread = std::thread{[this, startup = std::move(startup)]() mutable {
        worker_run(std::move(startup));
    }};

    // Run until the context requested to stop.
    while (!stop_requested())
    {
        // Handle waiting on all the sd-events.
        details::wait_process_completion::wait_once(*this);
    }

    // Stop has been requested, so finish up the loop.
    loop.finish();
    if (worker_thread.joinable())
    {
        worker_thread.join();
    }
}

void context::worker_run(task<> startup)
{
    // Begin the 'startup' task.
    // This shouldn't start detached because we want to be able to forward
    // failures back to the 'run'.  execution::ensure_started isn't
    // implemented yet, so we don't have a lot of other options.
    execution::start_detached(std::move(startup));

    // Also start up the sdbus 'wait/process' loop.
    execution::start_detached(details::wait_process_completion::loop(*this));

    // Run the execution::run_loop to handle all the tasks.
    loop.run();
}

void details::wait_process_completion::arm() noexcept
{
    // Call process.  True indicates something was handled and we do not
    // need to `wait`, because there might be yet another pending operation
    // to process, so immediately signal the operation as complete.
    if (ctx.get_bus().process_discard())
    {
        this->complete();
        return;
    }

    // We need to call wait now, get the current timeout and stage ourselves
    // as the next completion.

    // Get the bus' timeout.
    uint64_t to_usec = 0;
    sd_bus_get_timeout(get_busp(ctx.get_bus()), &to_usec);

    if (to_usec == UINT64_MAX)
    {
        // sd_bus_get_timeout returns UINT64_MAX to indicate 'wait forever'.
        // Turn this into -1 for sd-event.
        timeout = std::chrono::microseconds{-1};
    }
    else
    {
        timeout = std::chrono::microseconds(to_usec);
    }

    // Assign ourselves as the pending completion and release the caller.
    std::lock_guard lock{ctx.lock};
    ctx.staged = this;
    ctx.caller_wait.notify_one();
}

void details::wait_process_completion::wait_once(context& ctx)
{
    // Scope for lock.
    {
        std::unique_lock lock{ctx.lock};

        // If there isn't a completion waiting already, wait on the condition
        // variable for one to show up (we can't call `poll` yet because we
        // don't have the required parameters).
        ctx.caller_wait.wait(lock, [&] {
            return (ctx.pending != nullptr) || (ctx.staged != nullptr) ||
                   (ctx.stop_requested());
        });

        // Save the waiter as pending.
        if (ctx.pending == nullptr)
        {
            ctx.pending = std::exchange(ctx.staged, nullptr);
        }
    }

    // If the context has been requested to be stopped, exit now instead of
    // running the context event loop.
    if (ctx.stop_requested())
    {
        return;
    }

    // Run the event loop to process one request.
    ctx.event_loop.run_one(ctx.pending->timeout);

    // If there is a stop requested, we need to stop the pending operation.
    if (ctx.stop_requested())
    {
        decltype(ctx.pending) pending = nullptr;

        {
            std::lock_guard lock{ctx.lock};
            pending = std::exchange(ctx.pending, nullptr);
        }

        // Do the stop outside the lock to prevent potential deadlocks due to
        // the stop handler running.
        if (pending != nullptr)
        {
            pending->stop();
        }
    }
}

int context::dbus_event_handle(sd_event_source*, int, uint32_t, void* data)
{
    auto self = static_cast<context*>(data);

    decltype(self->pending) pending = nullptr;
    {
        std::lock_guard lock{self->lock};
        pending = std::exchange(self->pending, nullptr);
    }

    // Outside the lock complete the pending operation.
    //
    // This can cause the Receiver task (the worker) to start executing (on
    // this thread!), hence we do not want the lock held in order to avoid
    // deadlocks.
    if (pending != nullptr)
    {
        if (self->stop_requested())
        {
            pending->stop();
        }
        else
        {
            pending->complete();
        }
    }

    return 0;
}

} // namespace sdbusplus::async
