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

void scope::ended_task(std::exception_ptr&& e) noexcept
{
    scope_ns::scope_completion* p = nullptr;

    {
        std::lock_guard l{lock};
        --pending_count; // decrement count.

        if (e && pending_exception)
        {
            // Received a second exception without delivering the first
            // to a pending completion.  Terminate using the first one.
            try
            {
                std::rethrow_exception(std::exchange(pending_exception, {}));
            }
            catch (...)
            {
                std::terminate();
            }
        }

        // If the scope is complete, get the pending completion, if it exists.
        if (e || (pending_count == 0))
        {
            p = std::exchange(pending, nullptr);
        }

        // If we have an exception but no pending completion, save it away.
        if (e && !p)
        {
            pending_exception = std::move(e);
        }
    }

    if (p)
    {
        p->complete(std::move(e));
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
        if (s.pending_count == 0)
        {
            done = true;
        }
        else if (s.pending_exception)
        {
            e = std::exchange(s.pending_exception, {});
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
