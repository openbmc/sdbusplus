#include <sys/eventfd.h>
#include <unistd.h>

#include <sdbusplus/event.hpp>
#include <sdbusplus/exception.hpp>

namespace sdbusplus::event
{

source::source(source&& s)
{
    if (&s == this)
    {
        return;
    }
    ev = std::exchange(s.ev, nullptr);
    sourcep = std::exchange(s.sourcep, nullptr);
}

source& source::operator=(source&& s)
{
    if (nullptr != sourcep)
    {
        auto l = ev->obtain_lock();
        sd_event_source_unref(sourcep);
    }
    ev = std::exchange(s.ev, nullptr);
    sourcep = std::exchange(s.sourcep, nullptr);

    return *this;
}

source::~source()
{
    if (nullptr != sourcep)
    {
        auto l = ev->obtain_lock();
        sd_event_source_unref(sourcep);
    }
}

condition::condition(condition&& c)
{
    if (&c == this)
    {
        return;
    }

    condition_source = std::move(c.condition_source);
    fd = std::exchange(c.fd, -1);
}

condition& condition::operator=(condition&& c)
{
    condition_source = std::move(c.condition_source);
    if (fd >= 0)
    {
        close(fd);
    }
    fd = std::exchange(c.fd, -1);

    return *this;
}

void condition::signal()
{
    uint64_t value = 1;
    auto rc = write(fd, &value, sizeof(value));
    if (rc < static_cast<decltype(rc)>(sizeof(value)))
    {
        throw exception::SdBusError(errno, __func__);
    }
}

void condition::ack()
{
    uint64_t value = 0;
    auto rc = read(fd, &value, sizeof(value));
    if (rc < static_cast<decltype(rc)>(sizeof(value)))
    {
        throw exception::SdBusError(errno, __func__);
    }
}

event::event()
{
    if (auto rc = sd_event_new(&eventp); rc < 0)
    {
        throw exception::SdBusError(-rc, __func__);
    }
    run_condition = add_condition(run_wakeup, this);
}

void event::run_one(time_resolution timeout)
{
    auto l = obtain_lock<false>();

    auto rc = sd_event_run(eventp, static_cast<uint64_t>(timeout.count()));
    if (rc < 0)
    {
        throw exception::SdBusError(-rc, __func__);
    }
}

void event::break_run()
{
    run_condition.signal();
}

source event::add_io(int fd, uint32_t events, sd_event_io_handler_t handler,
                     void* data)
{
    auto l = obtain_lock();

    source s{*this};

    auto rc = sd_event_add_io(eventp, &s.sourcep, fd, events, handler, data);
    if (rc < 0)
    {
        throw exception::SdBusError(-rc, __func__);
    }

    return s;
}

condition event::add_condition(sd_event_io_handler_t handler, void* data)
{
    // We don't need any locks here because we only touch the sd_event
    // indirectly through `add_io` which handles its own locking.

    auto fd = eventfd(0, 0);
    if (fd < 0)
    {
        throw exception::SdBusError(errno, __func__);
    }

    try
    {
        auto io = add_io(fd, EPOLLIN, handler, data);
        return {std::move(io), std::move(fd)};
    }
    catch (...)
    {
        close(fd);
        throw;
    }
}

source event::add_oneshot_timer(sd_event_time_handler_t handler, void* data,
                                time_resolution time, time_resolution accuracy)
{
    auto l = obtain_lock();

    source s{*this};

    auto rc = sd_event_add_time_relative(
        eventp, &s.sourcep, CLOCK_BOOTTIME, time.count(), accuracy.count(),
        handler, data);

    if (rc < 0)
    {
        throw exception::SdBusError(-rc, __func__);
    }

    return s;
}

int event::run_wakeup(sd_event_source*, int, uint32_t, void* data)
{
    auto self = static_cast<event*>(data);
    self->run_condition.ack();

    return 0;
}

template <bool Signal>
std::unique_lock<std::recursive_mutex> event::obtain_lock()
{
    std::unique_lock stage{this->obtain_lock_stage};

    std::unique_lock<std::recursive_mutex> l{this->lock, std::defer_lock_t()};
    if constexpr (Signal)
    {
        if (!l.try_lock())
        {
            run_condition.signal();
            l.lock();
        }
    }
    else
    {
        l.lock();
    }

    return l;
}

} // namespace sdbusplus::event
