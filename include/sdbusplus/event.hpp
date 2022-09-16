#pragma once

#include <systemd/sd-event.h>

#include <chrono>
#include <iostream>
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
    explicit condition(event& e) : condition_source(e){};
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
    event();
    event(const event&) = delete;
    event(event&& e) = delete;

    ~event()
    {
        sd_event_unref(eventp);
    }

    /** Execute a single iteration of the run-loop (see sd_event_run). */
    void run_one(
        std::chrono::microseconds timeout = std::chrono::microseconds::max());
    /** Force a pending `run_one` to exit. */
    void break_run();

    /** Add a file-descriptor source to the sd-event (see sd_event_add_io). */
    source add_io(int fd, uint32_t events, sd_event_io_handler_t handler,
                  void* data);

    /** Add a eventfd-based sdbusplus::event::condition to the run-loop. */
    condition add_condition(sd_event_io_handler_t handler, void* data);

    /** Add a one shot timer source to the run-loop. */
    source add_oneshot_timer(
        sd_event_time_handler_t handler, void* data,
        std::chrono::microseconds time,
        std::chrono::microseconds accuracy = std::chrono::milliseconds(1));

    friend source;

  private:
    static int run_wakeup(sd_event_source*, int, uint32_t, void*);

    sd_event* eventp = nullptr;

    // Condition to allow 'break_run' to exit the run-loop.
    condition run_condition{*this};
    std::mutex lock{};

    // Safely get the lock, possibly signaling the running 'run_one' to exit.
    template <bool Signal = true>
    std::unique_lock<std::mutex> obtain_lock();
};

} // namespace event

using event_t = event::event;
using event_source_t = event::source;
using event_cond_t = event::condition;

} // namespace sdbusplus
