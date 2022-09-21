#include <sdbusplus/async/scope.hpp>

#include <exception>

namespace sdbusplus::async
{
scope::~scope() noexcept(false)
{
    if (pending_count != 0)
    {
        throw std::logic_error(
            "sdbusplus::async::scope destructed while tasks are pending.");
    }
}

void scope::started_task() noexcept
{
    std::lock_guard l{lock};
    ++pending_count;
}

void scope::ended_task() noexcept
{
    scope_ns::scope_completion* p = nullptr;

    {
        std::lock_guard l{lock};
        --pending_count;

        if (pending_count == 0)
        {
            p = std::exchange(pending, nullptr);
        }
    }

    if (p)
    {
        p->complete();
    }
}

scope_ns::scope_sender scope::empty() noexcept
{
    return scope_ns::scope_sender(*this);
}

namespace scope_ns
{
void scope_completion::arm() noexcept
{
    bool done = false;

    {
        std::lock_guard l{s.lock};
        if (s.pending_count == 0)
        {
            done = true;
        }
        else
        {
            s.pending = this;
        }
    }

    if (done)
    {
        this->complete();
    }
}
} // namespace scope_ns

} // namespace sdbusplus::async
