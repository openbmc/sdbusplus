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
        if (owned)
        {
            mutexInstance.unlock();
            owned = false;
        }
    }

    auto lock() noexcept;
    auto unlock() noexcept;

  private:
    mutex& mutexInstance;
    bool owned = false;
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

    void start() noexcept;

  private:
    virtual void complete() noexcept = 0;
    virtual void stop() noexcept = 0;

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
    using sender_concept = execution::sender_t;

    mutex_sender() = delete;
    explicit mutex_sender(mutex& mutexInstance) noexcept :
        mutexInstance(mutexInstance) {};

    template <typename Self, class... Env>
    static constexpr auto get_completion_signatures(Self&&, Env&&...)
        -> execution::completion_signatures<execution::set_value_t(),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    auto connect(R r) -> mutex_operation<R>
    {
        return {mutexInstance, std::move(r)};
    }

  private:
    mutex& mutexInstance;
};

} // namespace mutex_ns

inline auto lock_guard::lock() noexcept
{
    owned = true;
    return mutex_ns::mutex_sender{this->mutexInstance};
}

inline auto lock_guard::unlock() noexcept
{
    if (owned)
    {
        mutexInstance.unlock();
        owned = false;
    }
}

} // namespace sdbusplus::async
