#include <poll.h>
#include <systemd/sd-bus.h>

#include <sdbusplus/async/context.hpp>

#include <chrono>

/* The context run-loop is a careful handshake between two threads.
 *
 * 1. The thread which calls `context::run`, typically the `main`.
 * 2. The thread which is executing the context tasks.
 *
 * We want all dbus operations to be performed in the context execution task
 * (#2), but it is not allowed to block (ie. call it cannot `bus.wait()`).  We
 * therefore have the execution task call `bus.process()` but then signal to the
 * runner task (#1) when a `bus.wait()`-type operation is necessary.
 *
 * The process loop and wait interaction is defined as a Sender operation.  A
 * single instance of the Sender will call `bus.process()` and (if needed) block
 * completion of the process task until the wait completes.  The task is
 * implemented in `loop_process_runner` as basically a `while(1) co_await
 * Sender;` loop.
 *
 * One caveat to the `bus.wait()` operation is that, since sd_bus is not
 * thread-safe, it is unsafe for any threads outside of the task executor (#2)
 * to call bus-operations.  The Sender therefore determines the inputs to the
 * `poll` system-call, from the bus object, and forwards them to the Waiter.
 */

namespace sdbusplus::async::context
{

// Virtual class to handle the run-loop Sender completion.
struct context::run_loop_completion
{
    virtual ~run_loop_completion(){};
    virtual void complete() = 0;

    pollfd fd{};
    int timeout = 0;
};

namespace details
{

// Implementation (templated based on Receiver) of run_loop_completion.
template <execution::receiver_of<> Receiver>
struct run_loop_operation :
    context::run_loop_completion,
    bus::details::bus_friend
{
    run_loop_operation(context_t& ctx, Receiver r) :
        ctx(ctx), receiver(std::move(r))
    {}

    void complete() override final
    {
        execution::set_value(std::move(receiver));
    }

    void start() noexcept
    {
        // Call process.  True indicates something was handled and we do not
        // need to `wait`, so immediate signal the Sender as complete.
        if (ctx.get_bus().process_discard())
        {
            this->complete();
        }
        else // Need to wait.
        {
            // Get the bus' pollfd data.
            auto b = get_busp(ctx.get_bus());
            fd = {sd_bus_get_fd(b), static_cast<short>(sd_bus_get_events(b)),
                  0};

            // Get the bus timeout.
            uint64_t to_nsec = 0;
            sd_bus_get_timeout(b, &to_nsec);

            if (to_nsec == UINT64_MAX)
            {
                // sd_bus_get_timeout returns UINT64_MAX to indicate 'wait
                // forever'.  Turn this into a negative number for `poll`.
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

            // Set ourself as the pending Sender and signal the Waiter to run.
            std::unique_lock<std::mutex> lock{ctx.run_loop_lock};
            ctx.run_loop_complete = this;
            ctx.run_loop_wait.notify_one();
        }
    }

    context_t& ctx;
    Receiver receiver;
};

} // namespace details
namespace
{

struct run_loop_sender : execution::sender_of<false>
{
    explicit run_loop_sender(context_t& ctx) : ctx(ctx){};

    auto connect(execution::receiver_of<> auto receiver) noexcept
    {
        return details::run_loop_operation(ctx, std::move(receiver));
    }

  private:
    context_t& ctx;
};

auto loop_process_runner(context_t& ctx) -> execution::task<void>
{
    while (1)
    {
        // Perform a `bus.process()` via the Sender.
        co_await run_loop_sender(ctx);

        // Completion could be called from the thread which is performing
        // `bus.wait()`, but we need to ensure we are only executing in the
        // context execution thread.  Scheduler ourself onto that thread.
        co_await execution::schedule(ctx.get_scheduler());
    }
}

} // namespace

void context::run()
{
    // Place the `process/wait` loop onto the context for execution.
    exec_scope.spawn_on(get_scheduler(), execution::defer([this] {
                            return loop_process_runner(*this);
                        }));

    // Start handling the 'bus.wait()` side of the handshake.
    while (1)
    {
        run_loop_completion* c = nullptr;

        // Scope for lock.
        {
            std::unique_lock<std::mutex> lock{run_loop_lock};

            // If there isn't a Sender waiting already, wait on the condition
            // variable for one to show up (we can't call `poll` yet because we
            // don't have the required parameters).
            if (run_loop_complete == nullptr)
            {
                run_loop_wait.wait(lock);
            }

            // Save the waiter and call `poll`.
            std::swap(run_loop_complete, c);
            poll(&c->fd, 1, c->timeout);
        }

        // Outside the lock call complete; this can cause the Sender task to
        // start executing, hence why we do not want the lock held (even though
        // it would be safe in this case because we do a re-schedule operation,
        // we keep this in place so people implementing other Senders and using
        // this as a pattern don't make a mistake leading to deadlocks).
        c->complete();
    }
}

} // namespace sdbusplus::async::context
