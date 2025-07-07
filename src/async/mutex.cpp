#include <sdbusplus/async/mutex.hpp>

namespace sdbusplus::async
{

mutex::mutex(const std::string& name) : name(name) {}

void mutex::unlock()
{
    std::unique_lock l{lock};
    if (waitingTasks.empty())
    {
        auto wasLocked = locked.exchange(false);
        if (!wasLocked)
        {
            try
            {
                throw std::runtime_error("mutex is not locked!");
            }
            catch (...)
            {
                std::terminate();
            }
        }
        return;
    }
    // Wake up the next waiting task
    auto completion = std::move(waitingTasks.front());
    waitingTasks.pop();
    completion->complete();
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
    std::unique_lock l{mutexInstance.lock};
    if (tryLock() == true)
    {
        complete();
        return;
    }

    mutexInstance.waitingTasks.push(this);
}

} // namespace mutex_ns

} // namespace sdbusplus::async
