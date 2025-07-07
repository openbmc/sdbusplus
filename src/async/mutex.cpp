#include <sdbusplus/async/mutex.hpp>

namespace sdbusplus::async
{

mutex::mutex(context& ctx, const std::string& name) :
    context_ref(ctx), name(name)
{}

void mutex::unlock()
{
    // Set the locked state to false and notify the next waiting task.
    auto wasLocked = locked.exchange(false);
    if (wasLocked)
    {
        if (waitingTasks.empty())
        {
            return;
        }

        auto completion = std::move(waitingTasks.front());
        waitingTasks.pop();
        completion->complete();
    }
}

namespace mutex_ns
{

bool mutex_completion::tryLock()
{
    bool unLocked = false;
    return mutexInstance.locked.compare_exchange_strong(unLocked, true);
}

void mutex_completion::arm() noexcept
{
    if (mutexInstance.locked.load() == false)
    {
        complete();
        return;
    }

    mutexInstance.waitingTasks.push(this);
}

} // namespace mutex_ns

} // namespace sdbusplus::async
