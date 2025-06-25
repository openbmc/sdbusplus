#include <sdbusplus/async/defer.hpp>

namespace sdbusplus::async
{
defer::defer(context& ctx) : context_ref(ctx)
{
    static auto eventHandler =
        [](sd_event_source*, void* data) noexcept {
            static_cast<defer*>(data)->handleEvent();
            return 0;
        };

    try
    {
        source = event_loop().add_defer(eventHandler, this);
    }
    catch (...)
    {
        throw std::runtime_error("Failed to add defer to event loop");
    }
}

void defer::handleEvent() noexcept
{
    std::unique_lock l{lock};
    if (complete == nullptr)
    {
        pending = true;
        return;
    }
    auto c = std::exchange(complete, nullptr);
    l.unlock();
    c->complete();
}

namespace defer_ns
{

defer_completion::~defer_completion()
{
    std::unique_lock l{deferInstance.lock};

    if (deferInstance.complete == this)
    {
        std::exchange(deferInstance.complete, nullptr);
    }
}

void defer_completion::arm() noexcept
{
    // Set ourselves as the awaiting Receiver
    std::unique_lock l{deferInstance.lock};

    if (std::exchange(deferInstance.complete, this) != nullptr)
    {
        // We do not support two awaiters; throw exception. Since we are in
        // a noexcept context this will std::terminate anyhow, which is
        // approximately the same as 'assert' but with better information.
        try
        {
            throw std::logic_error("Defer already has an awaiting task");
        }
        catch (...)
        {
            std::terminate();
        }
    }

    // If we are already pending, we can complete immediately.
    if (std::exchange(deferInstance.pending, false))
    {
        l.unlock();
        this->complete();
    }
}

} // namespace defer_ns

} // namespace sdbusplus::async