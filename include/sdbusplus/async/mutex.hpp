#pragma once

#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/event.hpp>

#include <queue>
#include <string>

namespace sdbusplus::async
{

namespace mutex_ns
{
struct mutex_completion;
} // namespace mutex_ns

class lock_guard;

class mutex
{
  public:
    mutex() = delete;
    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;
    mutex(mutex&&) = delete;
    mutex& operator=(mutex&&) = delete;
    ~mutex() = default;

    mutex(const std::string& name = "sdbusplus::async::mutex");

    friend mutex_ns::mutex_completion;
    friend lock_guard;

  private:
    void unlock();

    std::string name;
    bool locked{false};
    std::queue<mutex_ns::mutex_completion*> waitingTasks;
    std::mutex lock{};
};

// RAII wrapper for mutex for the duration of a scoped block.
class lock_guard
{
  public:
    lock_guard() = delete;
    lock_guard(const lock_guard&) = delete;
    lock_guard& operator=(const lock_guard&) = delete;
    lock_guard(lock_guard&&) = delete;
    lock_guard& operator=(lock_guard&&) = delete;

    explicit lock_guard(mutex& mutexInstance) : mutexInstance(mutexInstance) {}

    ~lock_guard()
    {
        if (mutexInstance.locked)
        {
            mutexInstance.unlock();
        }
    }

    auto lock() noexcept;

  private:
    mutex& mutexInstance;
};

namespace mutex_ns
{

struct mutex_completion
{
    mutex_completion() = delete;
    mutex_completion(const mutex_completion&) = delete;
    mutex_completion& operator=(const mutex_completion&) = delete;
    mutex_completion(mutex_completion&&) = delete;
    ~mutex_completion() = default;

    explicit mutex_completion(mutex& mutexInstance) noexcept :
        mutexInstance(mutexInstance) {};

    friend mutex;

    friend void tag_invoke(execution::start_t, mutex_completion& self) noexcept
    {
        self.arm();
    }

  private:
    virtual void complete() noexcept = 0;
    virtual void stop() noexcept = 0;
    void arm() noexcept;

    mutex& mutexInstance;
};

// Implementation (templated based on Receiver) of mutex_completion.
template <execution::receiver Receiver>
struct mutex_operation : mutex_completion
{
    mutex_operation(mutex& mutexInstance, Receiver r) :
        mutex_completion(mutexInstance), receiver(std::move(r))
    {}

  private:
    void complete() noexcept override final
    {
        execution::set_value(std::move(receiver));
    }

    void stop() noexcept override final
    {
        execution::set_stopped(std::move(receiver));
    }

    Receiver receiver;
};

// mutex sender
struct mutex_sender
{
    using is_sender = void;

    mutex_sender() = delete;
    explicit mutex_sender(mutex& mutexInstance) noexcept :
        mutexInstance(mutexInstance) {};

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const mutex_sender&, auto)
        -> execution::completion_signatures<execution::set_value_t(),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, mutex_sender&& self, R r)
        -> mutex_operation<R>
    {
        return {self.mutexInstance, std::move(r)};
    }

  private:
    mutex& mutexInstance;
};

} // namespace mutex_ns

inline auto lock_guard::lock() noexcept
{
    return mutex_ns::mutex_sender{this->mutexInstance};
}

} // namespace sdbusplus::async
