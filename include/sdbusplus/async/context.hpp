#pragma once

#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/bus.hpp>

#include <condition_variable>
#include <mutex>
#include <type_traits>

namespace sdbusplus::async
{

namespace context
{

namespace details
{
template <execution::receiver_of<> Receiver>
struct run_loop_operation;
}

/** @class context
 *
 *  Provides an async context (and scheduler) for managing a bus connection.
 *  The initial service task is provided to the `startup` method, which runs
 *  according to a specified policy.  Once `run` is called, the context will
 *  execute the typical `bus.process`/`bus.wait` until the conditions for the
 *  `startup` specified policy are met.
 *
 *  A note on thread-safety:
 *
 *  The context runs all scheduled tasks, except for the main run-loop, in a
 *  separate thread from the one which constructed the context.  The context
 *  handles thread-safety such that `run` can be called from the constructing
 *  thread, but NO OTHER operations should be called from the constructing
 *  thread.  All other operations are assumed to be called from an async context
 *  managed by the context itself (such as the `startup` routine scheduling
 *  child operations).
 *
 *  Even something as trivial as interacting with the `context_t::get_bus()`
 *  result from the constructing thread can cause your program to crash because
 *  sd_bus itself is not thread-safe!
 *
 *  The typical program structure of using the context is:
 *  ```
 *  int main()
 *  {
 *      context_t ctx{};
 *      ctx.startup(...lambda to run async...);
 *      ctx.run();
 *      return 0;
 *  }
 *  ```
 */
class context
{
  public:
    /** Policies for `startup` to indicate when the context/service should stop.
     *
     *  TODO: Not currently supported.
     */
    enum run_policy
    {
        run_single,        //< Will run until the `startup` completes.
        run_until_stopped, //< Will run until stopped.
    };

    context(const context&) = delete;
    context& operator=(const context&) = delete;
    context(context&&) = delete;
    context& operator=(context&&) = delete;
    ~context() = default;

    /** Construct the context and assign it a `bus` connection. */
    explicit context(sdbusplus::bus_t&& bus = sdbusplus::bus::new_bus()) :
        bus(std::move(bus))
    {}

    /** Execute the process/wait loop until completion. */
    void run();

    /** Provide the initial startup task.
     *
     *  @param[in] routine - A Callable that returns a Sender/Task to be
     *                       executed inside the async context.
     *  @param[in] run_policy - The policy to determine when the context should
     *                          be complete.
     *
     *  Note: the `routine` can begin execution immediately on a different
     *        thread than what is called with `run`.
     */
    void startup(execution::returns_sender auto&& routine,
                 run_policy = run_policy::run_single)
    {
        exec_scope.spawn_on(get_scheduler(),
                            execution::defer(std::move(routine)));
    }

    /** Get the managed bus. */
    auto& get_bus() noexcept
    {
        return bus;
    };

    /** Get the task scheduler. */
    decltype(auto) get_scheduler() noexcept
    {
        return exec_ctx.get_scheduler();
    }

    // Forward / friend declarations.
    template <execution::receiver_of<> Receiver>
    friend struct details::run_loop_operation;
    struct run_loop_completion;

  private:
    bus_t bus;
    execution::async_scope exec_scope{};
    execution::timed_single_thread_context exec_ctx{};

    std::mutex run_loop_lock;
    std::condition_variable run_loop_wait;
    run_loop_completion* run_loop_complete = nullptr;
};
} // namespace context

using context_t = context::context;

} // namespace sdbusplus::async
