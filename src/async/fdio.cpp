#include <poll.h>

#include <sdbusplus/async/fdio.hpp>

namespace sdbusplus::async
{
fdio::fdio(context& ctx, int fd, std::chrono::microseconds initialTimeout) :
    context_ref(ctx),
    timeout(
        std::chrono::duration_cast<event_t::time_resolution>(initialTimeout)),
    ioFd(fd)
{
    static auto eventHandler =
        [](sd_event_source*, int, uint32_t, void* data) noexcept {
            static_cast<fdio*>(data)->handleEvent();
            return 0;
        };

    auto eventLock = event_loop().acquire_lock();
    try
    {
        ioSource = event_loop().add_io(fd, EPOLLIN, eventHandler, this);
        ioSource.set_enabled(false);
    }
    catch (...)
    {
        ioSource.reset_locked();
        throw std::runtime_error("Failed to add fd to event loop");
    }
}

fdio::~fdio() noexcept
{
    auto eventLock = event_loop().acquire_lock();
    retireSourcesLocked();
    complete = nullptr;
}

void fdio::setTimeout(std::chrono::microseconds newTimeout)
{
    auto eventLock = event_loop().acquire_lock();
    timeout = std::chrono::duration_cast<event_t::time_resolution>(newTimeout);
}

std::exception_ptr fdio::retireSourcesLocked() noexcept
{
    std::exception_ptr failure;
    try
    {
        ioSource.set_enabled(false);
    }
    catch (...)
    {
        failure = std::current_exception();
        terminalError = failure;
        ioSource.reset_locked();
    }

    timerSource.reset_locked();
    return failure;
}

std::exception_ptr fdio::registerTimerLocked(
    event_t::time_resolution duration) noexcept
{
    static auto eventHandler =
        [](sd_event_source*, uint64_t, void* data) noexcept {
            static_cast<fdio*>(data)->handleTimeout();
            return 0;
        };

    try
    {
        timerSource =
            event_loop().add_oneshot_timer(eventHandler, this, duration);
    }
    catch (...)
    {
        auto failure = std::current_exception();
        timerSource.reset_locked();
        return failure;
    }

    return nullptr;
}

void fdio::handleEvent() noexcept
{
    auto eventLock = event_loop().acquire_lock();
    auto* completion = complete;
    if (completion == nullptr)
    {
        retireSourcesLocked();
        return;
    }

    auto failure = retireSourcesLocked();
    complete = nullptr;
    if (failure)
    {
        completion->error(failure);
    }
    else
    {
        completion->complete();
    }
}

void fdio::handleTimeout() noexcept
{
    auto eventLock = event_loop().acquire_lock();
    auto* completion = complete;
    if (completion == nullptr)
    {
        retireSourcesLocked();
        return;
    }

    // Data may have arrived on the fd before the timer fired. When both
    // sources are ready simultaneously, sd-event may dispatch the timer
    // first, so preserve the readable-before-timeout behavior.
    struct pollfd pfd = {ioFd, POLLIN, 0};
    const bool readable = ::poll(&pfd, 1, 0) > 0 && (pfd.revents & POLLIN);

    auto failure = retireSourcesLocked();
    complete = nullptr;
    if (failure)
    {
        completion->error(failure);
    }
    else if (readable)
    {
        completion->complete();
    }
    else
    {
        std::exception_ptr timeoutError;
        try
        {
            throw fdio_timeout_exception();
        }
        catch (...)
        {
            timeoutError = std::current_exception();
        }
        completion->error(timeoutError);
    }
}

namespace fdio_ns
{

fdio_completion::~fdio_completion()
{
    auto eventLock = fdioInstance.event_loop().acquire_lock();
    if (fdioInstance.complete == this)
    {
        fdioInstance.retireSourcesLocked();
        fdioInstance.complete = nullptr;
    }
}

void fdio_completion::start() noexcept
{
    auto eventLock = fdioInstance.event_loop().acquire_lock();

    if (fdioInstance.complete != nullptr)
    {
        // Preserve the single-awaiting-task contract without disturbing the
        // existing wait or either of its sources.
        try
        {
            throw std::logic_error(
                "fdio_completion started with another await already pending!");
        }
        catch (...)
        {
            std::terminate();
        }
    }

    if (fdioInstance.terminalError)
    {
        auto failure = fdioInstance.terminalError;
        error(failure);
        return;
    }

    const auto waitTimeout = fdioInstance.timeout;
    if (waitTimeout != event_t::time_resolution::zero())
    {
        auto failure = fdioInstance.registerTimerLocked(waitTimeout);
        if (failure)
        {
            error(failure);
            return;
        }
    }

    fdioInstance.complete = this;
    try
    {
        fdioInstance.ioSource.set_enabled(true);
    }
    catch (...)
    {
        auto failure = std::current_exception();
        fdioInstance.terminalError = failure;
        fdioInstance.ioSource.reset_locked();
        fdioInstance.timerSource.reset_locked();
        fdioInstance.complete = nullptr;
        error(failure);
        return;
    }
}

} // namespace fdio_ns

} // namespace sdbusplus::async
