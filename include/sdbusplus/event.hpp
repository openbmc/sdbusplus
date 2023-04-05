#pragma once

#include <systemd/sd-event.h>

#include <chrono>
#include <mutex>
#include <utility>

namespace sdbusplus
{
namespace event
{
class event;

/** RAII holder for sd_event_sources */
class source
{
  public:
    friend event;

    source() = default;
    explicit source(event& e) : ev(&e) {}

    source(const source&) = delete;
    source(source&&);
    source& operator=(const source&) = delete;
    source& operator=(source&&);
    ~source();

  private:
    source(event& e, sd_event_source*& s) : ev(&e)
    {
        sourcep = std::exchange(s, nullptr);
    }

    event* ev = nullptr;
    sd_event_source* sourcep = nullptr;
};

/** sd-event wrapper for eventfd
 *
 *  This can be used to create something similar to a std::condition_variable
 *  but backed by sd-event.
 */
class condition
{
  public:
    friend event;

    condition() = delete;
    explicit condition(event& e) : condition_source(e) {}
    condition(const condition&) = delete;
    condition(condition&&);

    condition& operator=(const condition&) = delete;
    condition& operator=(condition&&);

    ~condition()
    {
        if (fd >= 0)
        {
            close(fd);
        }
    }

    /** Increment the signal count on the eventfd. */
    void signal();
    /** Acknowledge all pending signals on the eventfd. */
    void ack();

  private:
    condition(source&& s, int&& f) :
        condition_source(std::move(s)), fd(std::exchange(f, -1))
    {}

    source condition_source;
    int fd = -1;
};

/** sd-event based run-loop implementation.
 *
 *  This is sd-event is thread-safe in the sense that one thread may be
 *  executing 'run_one' while other threads create (or destruct) additional
 *  sources.  This might result in the 'run_one' exiting having done no
 *  work, but the state of the underlying sd-event structures is kept
 *  thread-safe.
 */
class event
{
  public:
    using time_resolution = std::chrono::microseconds;

    event();
    event(const event&) = delete;
    event(event&& e) = delete;

    ~event()
    {
        sd_event_unref(eventp);
    }

    /** Execute a single iteration of the run-loop (see sd_event_run). */
    void run_one(time_resolution timeout = time_resolution::max());
    /** Force a pending `run_one` to exit. */
    void break_run();

    /** Add a file-descriptor source to the sd-event (see sd_event_add_io). */
    source add_io(int fd, uint32_t events, sd_event_io_handler_t handler,
                  void* data);

    /** Add a eventfd-based sdbusplus::event::condition to the run-loop. */
    condition add_condition(sd_event_io_handler_t handler, void* data);

    /** Add a one shot timer source to the run-loop. */
    source add_oneshot_timer(
        sd_event_time_handler_t handler, void* data, time_resolution time,
        time_resolution accuracy = std::chrono::milliseconds(1));

    friend source;

  private:
    static int run_wakeup(sd_event_source*, int, uint32_t, void*);

    sd_event* eventp = nullptr;

    // Condition to allow 'break_run' to exit the run-loop.
    condition run_condition{*this};

    // Lock for the sd_event.
    //
    // There are cases where we need to lock the mutex from inside the context
    // of a sd-event callback, while the lock is already held.  Use a
    // recursive_mutex to allow this.
    std::recursive_mutex lock{};

    // Safely get the lock, possibly signaling the running 'run_one' to exit.
    template <bool Signal = true>
    std::unique_lock<std::recursive_mutex> obtain_lock();
    // When obtain_lock signals 'run_one' to exit, we want a priority of
    // obtaining the lock so that the 'run_one' task doesn't run and reclaim
    // the lock before the signaller can run.  This stage is first obtained
    // prior to getting the primary lock in order to set an order.
    std::mutex obtain_lock_stage{};
};

} // namespace event

using event_t = event::event;
using event_source_t = event::source;
using event_cond_t = event::condition;

} // namespace sdbusplus
