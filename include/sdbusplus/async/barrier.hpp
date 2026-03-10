#pragma once

#include <sdbusplus/async/execution.hpp>

#include <mutex>
#include <stdexcept>
#include <vector>

namespace sdbusplus::async
{

namespace barrier_ns
{
struct barrier_completion;
} // namespace barrier_ns

/** @class barrier
 *  @brief An async barrier implementation.
 *
 *  A barrier allows a fixed number of tasks to coordinate and wait until
 *  they have all reached the barrier.
 */
class barrier
{
  public:
    barrier(const barrier&) = delete;
    barrier& operator=(const barrier&) = delete;
    barrier(barrier&&) = delete;
    barrier& operator=(barrier&&) = delete;
    ~barrier() = default;

    /** @brief Construct a barrier with a count.
     *
     *  @param[in] count - The number of tasks that must call wait.
     */
    explicit barrier(std::size_t count = 2) : count(count)
    {
        if (count < 1)
        {
            throw std::invalid_argument(
                "sdbusplus::async::barrier count must be greater than 0.");
        }
    }

    /** @brief Wait for the barrier to be reached.
     *
     *  @return A sender that completes when the barrier count is reached.
     */
    auto wait() noexcept;

    friend barrier_ns::barrier_completion;

  private:
    std::size_t count;
    std::vector<barrier_ns::barrier_completion*> waiters;
    std::mutex lock{};
};

namespace barrier_ns
{

struct barrier_completion
{
    barrier_completion() = delete;
    barrier_completion(const barrier_completion&) = delete;
    barrier_completion& operator=(const barrier_completion&) = delete;
    barrier_completion(barrier_completion&&) = delete;
    ~barrier_completion() = default;

    explicit barrier_completion(barrier& instance) noexcept :
        instance(instance) {};

    void start() noexcept;

  private:
    virtual void complete() noexcept = 0;

    barrier& instance;
};

template <execution::receiver Receiver>
struct barrier_operation : barrier_completion
{
    barrier_operation(barrier& instance, Receiver r) :
        barrier_completion(instance), receiver(std::move(r))
    {}

    void complete() noexcept override final
    {
        execution::set_value(std::move(receiver));
    }

    void start() noexcept
    {
        this->barrier_completion::start();
    }

  private:
    Receiver receiver;
};

struct barrier_sender
{
    using sender_concept = execution::sender_t;

    barrier_sender() = delete;
    explicit barrier_sender(barrier& instance) noexcept : instance(instance) {};

    template <typename Self, class... Env>
    static constexpr auto get_completion_signatures(Self&&, Env&&...)
        -> execution::completion_signatures<execution::set_value_t()>;

    template <execution::receiver R>
    auto connect(R r) -> barrier_operation<R>
    {
        return {instance, std::move(r)};
    }

  private:
    barrier& instance;
};

} // namespace barrier_ns

inline auto barrier::wait() noexcept
{
    return barrier_ns::barrier_sender{*this};
}

} // namespace sdbusplus::async
