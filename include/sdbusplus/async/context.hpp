#pragma once

#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/async/task.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/event.hpp>

#include <condition_variable>
#include <mutex>
#include <stop_token>
#include <thread>

namespace sdbusplus::async
{

namespace details
{
struct wait_process_completion;
struct context_friend;

} // namespace details

/** @brief A run-loop context for handling asynchronous dbus operations.
 *
 *  This class encapsulates the run-loop for asynchronous operations,
 *  especially those using co-routines.  Primarily, the object is given
 *  co-routines (or Senders) for the processing, via `spawn`, and then the
 *  object is `run` which handles all asynchronous operations until the
 *  context is stopped, via `request_stop`.
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
class context : public bus::details::bus_friend
{
  public:
    explicit context(bus_t&& bus = bus::new_bus());
    context(context&&) = delete;
    context(const context&) = delete;

    // The context destructor can throw because it will throw (and likely
    // terminate) if it has been improperly shutdown in order to avoid leaking
    // work.
    ~context() noexcept(false);

    /** Run the loop. */
    void run();

    /** Spawn a Sender to run on the context.
     *
     * @param[in] sender - The Sender to run.
     */
    template <typename Snd>
    void spawn(Snd&& sender)
    {
        check_stop_requested();

        pending_tasks.spawn(std::move(
            execution::starts_on(loop.get_scheduler(), std::move(sender))));

        spawn_watcher();
    }

    bus_t& get_bus() noexcept
    {
        return bus;
    }
    operator bus_t&() noexcept
    {
        return bus;
    }

    void request_name(const char* service)
    {
        name_requested = true;
        bus.request_name(service);
    }

    bool request_stop() noexcept
    {
        return initial_stop.request_stop();
    }
    bool stop_requested() noexcept
    {
        return initial_stop.stop_requested();
    }

    friend details::wait_process_completion;
    friend details::context_friend;

  private:
    bus_t bus;
    event_source_t dbus_source;
    event_t event_loop{};
    bool name_requested = false;

    /** The async run-loop from std::execution. */
    execution::run_loop loop{};
    /** The worker thread to handle async tasks. */
    std::thread worker_thread{};
    /** Stop source */
    std::stop_source initial_stop{};

    async_scope pending_tasks{};
    // In order to coordinate final completion of work, we keep some tasks
    // on a separate scope (the ones which maintain the sd-event/dbus state
    // and keep a final stop-source for them.
    async_scope internal_tasks{};
    std::stop_source final_stop{};

    // Lock and condition variable to signal `caller`.
    std::mutex lock{};
    std::condition_variable caller_wait{};

    bool spawn_watcher_running = false;

    /** Completion object to signal the worker that 'sd_bus_wait' is done. */
    details::wait_process_completion* staged = nullptr;
    details::wait_process_completion* pending = nullptr;
    bool wait_process_stopped = false;

    void worker_run();
    void spawn_complete();
    void check_stop_requested();
    void spawn_watcher();

    void caller_run();
    void wait_for_wait_process_stopped();

    static int dbus_event_handle(sd_event_source*, int, uint32_t, void*);
};

/** @brief Simple holder of a context&
 *
 *  A common pattern is for classes to hold a reference to a
 *  context.  Add a class that can be inherited instead of
 *  having as a class member.  This allows the context-ref constructor to be
 * placed earliest in the ctor initializer list, so that the reference can be
 * available from inherited classes (ex. for CRTP patterns).
 *
 */
class context_ref
{
  public:
    context_ref() = delete;
    explicit context_ref(context& ctx) : ctx(ctx) {}

  protected:
    context& ctx;
};

namespace details
{
struct context_friend
{
    static event_t& get_event_loop(context& ctx)
    {
        return ctx.event_loop;
    }

    static auto get_scheduler(context& ctx)
    {
        return ctx.loop.get_scheduler();
    }
};
} // namespace details

} // namespace sdbusplus::async
