#pragma once

#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/bus.hpp>

#include <condition_variable>
#include <mutex>
#include <thread>

namespace sdbusplus::async
{

namespace details
{
struct wait_process_completion;
}

/** @brief A run-loop context for handling asynchronous dbus operations.
 *
 *  This class encapsulates the run-loop for asynchronous operations,
 *  especially those using co-routines.  Primarily, the object is given
 *  a co-routine for the 'startup' processing of a daemon and it runs
 *  both the startup routine and any further asynchronous operations until
 *  the context is stopped.
 *
 *  TODO: Stopping is not currently supported.
 *
 *  The context has two threads:
 *      - The thread which called `run`, often from `main`, and named the
 *        `caller`.
 *      - A worker thread (`worker`), which performs the async operations.
 *
 *  In order to avoid blocking the worker needlessly, the `caller` is the only
 *  thread which called `sd_bus_wait`, but the `worker` is where all
 *  `sd_bus_process` calls are performed.  There is a condition-variable based
 *  handshake between the two threads to accomplish this interaction.
 */
class context
{
  public:
    explicit context(bus_t&& bus = bus::new_default()) : bus(std::move(bus)) {}

    // The context destructor can throw because it will throw (and likely
    // terminate) if it has been improperly shutdown in order to avoid leaking
    // work.
    ~context() noexcept(false);

    /** Run the loop.
     *
     *  @param[in] startup - The initialization operation to run.
     */
    template <execution::sender_of<> Snd>
    void run(Snd&& startup);

    bus_t& get_bus() noexcept
    {
        return bus;
    }

    friend struct details::wait_process_completion;

  private:
    bus_t bus;

    /** The async run-loop from std::execution. */
    execution::run_loop loop{};
    /** The worker thread to handle async tasks. */
    std::thread worker_thread{};

    // Lock and condition variable to signal `caller`.
    std::mutex lock{};
    std::condition_variable caller_wait{};

    /** Completion object to signal the worker that 'sd_bus_wait' is done. */
    details::wait_process_completion* complete = nullptr;

    void worker_run(task<> startup);
    void caller_run(task<> startup);
};

template <execution::sender_of<> Snd>
void context::run(Snd&& startup)
{
    // If Snd is a task, we can simply forward it on to the `caller_run`,
    // but if it is a generic Sender we need to do a transformation first.
    // In most cases, we expect users to use co-routines (ie. task<>).

    if constexpr (std::is_same_v<task<>, std::remove_cvref_t<Snd>>)
    {
        caller_run(std::forward<Snd>(startup));
    }
    else
    {
        // Transform the generic sender into a task by a simple lambda.

        caller_run([](auto&& s) -> task<> {
            co_await std::move(s);
            co_return;
        }(std::forward<Snd>(startup)));
    }
}

} // namespace sdbusplus::async
