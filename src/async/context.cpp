#include <systemd/sd-bus.h>

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/async/timer.hpp>

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
struct wait_process_completion : context_ref, bus::details::bus_friend
{
    explicit wait_process_completion(context& ctx) : context_ref(ctx) {}
    virtual ~wait_process_completion() = default;

    // Called by the `caller` to indicate the Sender is completed.
    virtual void complete() noexcept = 0;
    // Called by the `caller` to indicate the Sender should be stopped.
    virtual void stop() noexcept = 0;

    void start() noexcept;

    // Data to share with the worker.
    event_t::time_resolution timeout{};

    static auto loop(context& ctx) -> task<>;
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
        // Stop can be called when the context is shutting down,
        // so treat it as if the receiver completed.
        execution::set_value(std::move(this->receiver));
    }

    R receiver;
};

/* The sender for the wait/process event. */
struct wait_process_sender : public context_ref
{
    using sender_concept = execution::sender_t;

    explicit wait_process_sender(context& ctx) : context_ref(ctx) {}

    template <typename Self, class... Env>
    static constexpr auto get_completion_signatures(Self&&, Env&&...)
        -> execution::completion_signatures<execution::set_value_t()>;

    template <execution::receiver R>
    auto connect(R r) -> wait_process_operation<R>
    {
        // Create the completion for the wait.
        return {ctx, std::move(r)};
    }
};

auto wait_process_completion::loop(context& ctx) -> task<>
{
    while (!ctx.final_stop.stop_requested())
    {
        // Handle the next sdbus event.  Completion likely happened on a
        // different thread so we need to transfer back to the worker thread.
        co_await execution::continues_on(wait_process_sender(ctx),
                                         ctx.loop.get_scheduler());
    }

    {
        std::lock_guard lock{ctx.lock};
        ctx.wait_process_stopped = true;
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

void context::run()
{
    // Run the primary portion of the run-loop.
    caller_run();

    // This should be final_stop...

    // We need to wait for the pending wait process and stop it.
    wait_for_wait_process_stopped();

    // Wait for all the internal tasks to complete.
    stdexec::sync_wait(internal_tasks.on_empty());

    // Finish up the loop and join the thread.
    // (There shouldn't be anything going on by this point anyhow.)
    loop.finish();
    if (worker_thread.joinable())
    {
        worker_thread.join();
    }
}

static auto watchdog_loop(sdbusplus::async::context& ctx) -> task<>
{
    auto watchdog_time =
        std::chrono::microseconds(ctx.get_bus().watchdog_enabled());
    if (watchdog_time.count() == 0)
    {
        co_return;
    }

    // Recommended interval is half of WATCHDOG_USEC
    watchdog_time /= 2;

    while (!ctx.stop_requested())
    {
        ctx.get_bus().watchdog_pet();
        co_await sleep_for(ctx, watchdog_time);
    }
}

void context::worker_run()
{
    internal_tasks.spawn(watchdog_loop(*this));

    // Start the sdbus 'wait/process' loop; treat it as an internal task.
    internal_tasks.spawn(details::wait_process_completion::loop(*this));

    // Run the execution::run_loop to handle all the tasks.
    loop.run();
}

void context::spawn_complete()
{
    {
        std::lock_guard l{lock};
        spawn_watcher_running = false;
    }

    if (stop_requested())
    {
        final_stop.request_stop();
    }

    caller_wait.notify_one();
    event_loop.break_run();
}

void context::check_stop_requested()
{
    if (stop_requested())
    {
        throw std::logic_error(
            "sdbusplus::async::context spawn called while already stopped.");
    }
}

void context::spawn_watcher()
{
    {
        std::lock_guard l{lock};
        if (spawn_watcher_running)
        {
            return;
        }

        spawn_watcher_running = true;
    }

    // Spawn the watch for completion / exceptions.
    internal_tasks.spawn(pending_tasks.on_empty() |
                         execution::then([this]() { spawn_complete(); }));
}

void context::caller_run()
{
    // We are able to run the loop until the context is requested to stop or
    // we get an exception.
    auto keep_running = [this]() {
        std::lock_guard l{lock};
        return !final_stop.stop_requested();
    };

    // If we are suppose to keep running, start the run loop.
    if (keep_running())
    {
        // Start up the worker thread.
        if (!worker_thread.joinable())
        {
            worker_thread = std::thread{[this]() { worker_run(); }};
        }
        else
        {
            // We've already been running and there might a completion pending.
            // Spawn a new watcher that checks for these.
            spawn_watcher();
        }

        while (keep_running())
        {
            // Handle waiting on all the sd-events.
            details::wait_process_completion::wait_once(*this);
        }
    }
    else
    {
        // There might be pending completions still, so spawn a watcher for
        // them.
        spawn_watcher();
    }
}

void context::wait_for_wait_process_stopped()
{
    auto worker = std::exchange(pending, nullptr);
    while (worker == nullptr)
    {
        std::lock_guard l{lock};
        if (wait_process_stopped)
        {
            break;
        }

        worker = std::exchange(staged, nullptr);
        if (!worker)
        {
            std::this_thread::yield();
        }
    }
    if (worker)
    {
        worker->stop();
        wait_process_stopped = true;
    }
}

void details::wait_process_completion::start() noexcept
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
    sd_bus_get_timeout(get_busp(ctx), &to_usec);

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
                   ctx.final_stop.stop_requested();
        });

        // Save the waiter as pending.
        if (ctx.pending == nullptr)
        {
            ctx.pending = std::exchange(ctx.staged, nullptr);
        }
    }

    // Run the event loop to process one request.
    // If the context has been requested to be stopped, skip the event loop.
    if (!ctx.final_stop.stop_requested() && ctx.pending)
    {
        ctx.event_loop.run_one(ctx.pending->timeout);
    }
}

int context::dbus_event_handle(sd_event_source*, int, uint32_t, void* data)
{
    auto self = static_cast<context*>(data);

    auto pending = std::exchange(self->pending, nullptr);
    if (pending != nullptr)
    {
        pending->complete();
    }

    return 0;
}

} // namespace sdbusplus::async
