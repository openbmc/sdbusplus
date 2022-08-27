#include <poll.h>
#include <systemd/sd-bus.h>

#include <sdbusplus/async/context.hpp>

namespace sdbusplus::async
{

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

    // Arm the completion event.
    void arm() noexcept;

    // Data to share with the worker.
    context& ctx;
    pollfd fd{};
    int timeout = 0;

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

    void complete() noexcept override
    {
        execution::set_value(std::move(this->receiver));
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
    while (1)
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

void context::caller_run(task<> startup)
{
    // Start up the worker thread.
    worker_thread = std::thread{[this, startup = std::move(startup)]() mutable {
        worker_run(std::move(startup));
    }};

    while (1)
    {
        // Handle 'sd_bus_wait's.
        details::wait_process_completion::wait_once(*this);
    }

    // TODO: We can't actually get here.  Need to deal with stop conditions and
    // then we'll need this code.
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
    execution::start_detached(
        std::move(details::wait_process_completion::loop(*this)));

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

    // We need to call wait now, so formulate all the data that the 'caller'
    // needs.

    // Get the bus' pollfd data.
    auto b = get_busp(ctx.get_bus());
    fd = pollfd{sd_bus_get_fd(b), static_cast<short>(sd_bus_get_events(b)), 0};

    // Get the bus' timeout.
    uint64_t to_nsec = 0;
    sd_bus_get_timeout(b, &to_nsec);

    if (to_nsec == UINT64_MAX)
    {
        // sd_bus_get_timeout returns UINT64_MAX to indicate 'wait forever'.
        // Turn this into a negative number for `poll`.
        timeout = -1;
    }
    else
    {
        // Otherwise, convert usec from sd_bus to msec for poll.
        // sd_bus manpage suggests you should round-up (ceil).
        timeout = std::chrono::ceil<std::chrono::milliseconds>(
                      std::chrono::microseconds(to_nsec))
                      .count();
    }

    // Assign ourselves as the pending completion and release the caller.
    std::unique_lock lock{ctx.lock};
    ctx.complete = this;
    ctx.caller_wait.notify_one();
}

void details::wait_process_completion::wait_once(context& ctx)
{
    details::wait_process_completion* c = nullptr;

    // Scope for lock.
    {
        std::unique_lock lock{ctx.lock};

        // If there isn't a completion waiting already, wait on the condition
        // variable for one to show up (we can't call `poll` yet because we
        // don't have the required parameters).
        ctx.caller_wait.wait(lock, [&] { return ctx.complete != nullptr; });

        // Save the waiter and call `poll`.
        c = std::exchange(ctx.complete, nullptr);
        poll(&c->fd, 1, c->timeout);
    }

    // Outside the lock complete the operation; this can cause the Receiver
    // task (the worker) to start executing, hence why we do not want the
    // lock held.
    c->complete();
}

} // namespace sdbusplus::async
