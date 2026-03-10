#include <sdbusplus/async/barrier.hpp>

#include <ranges>

namespace sdbusplus::async::barrier_ns
{

void barrier_completion::start() noexcept
{
    std::unique_lock l{instance.lock};

    instance.waiters.push_back(this);

    if (instance.waiters.size() >= instance.count)
    {
        // Since completion can cause a task to resume, we need to
        // finish up our work with the barrier and then complete the tasks,
        // otherwise we can end up in a deadlock where the unique-lock is held
        // by this thread, but it is running another task that also attempts to
        // lock (when reaching the barrier a second time).
        decltype(instance.waiters) tasks;
        tasks.swap(instance.waiters);
        l.unlock();

        std::ranges::for_each(tasks, [](auto t) { t->complete(); });
    }
}

} // namespace sdbusplus::async::barrier_ns
