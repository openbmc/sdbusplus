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
    started = true;
    ++pending_count;
}

void scope::ended_task(std::exception_ptr&& e) noexcept
{
    scope_ns::scope_completion* p = nullptr;

    {
        std::lock_guard l{lock};
        --pending_count; // decrement count.

        if (e)
        {
            pending_exceptions.emplace_back(std::exchange(e, {}));
        }

        // If the scope is complete, get the pending completion, if it exists.
        if (!pending_exceptions.empty() || (pending_count == 0))
        {
            p = std::exchange(pending, nullptr);
        }
    }

    if (p)
    {
        std::exception_ptr fwd_exception = {};
        if (!pending_exceptions.empty())
        {
            fwd_exception = pending_exceptions.front();
            pending_exceptions.pop_front();
        }
        p->complete(std::move(fwd_exception));
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
    std::exception_ptr e{};

    {
        std::lock_guard l{s.lock};
        if (!s.started)
        {
            done = false;
        }
        else if (!s.pending_exceptions.empty())
        {
            e = s.pending_exceptions.front();
            s.pending_exceptions.pop_front();
            done = true;
        }
        else if (s.pending_count == 0)
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
        this->complete(std::move(e));
    }
}
} // namespace scope_ns

} // namespace sdbusplus::async
