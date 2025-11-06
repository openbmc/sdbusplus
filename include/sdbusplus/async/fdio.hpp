#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/event.hpp>

namespace sdbusplus::async
{

namespace fdio_ns
{
struct fdio_completion;
}

class fdio : private context_ref, details::context_friend
{
  public:
    fdio() = delete;
    fdio(const fdio&) = delete;
    fdio& operator=(const fdio&) = delete;
    fdio(fdio&&) = delete;
    fdio& operator=(fdio&&) = delete;
    ~fdio() = default;

    fdio(context& ctx, int fd,
         std::chrono::microseconds timeout = std::chrono::microseconds(0));

    /** Get the next fd event.
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `next`.
     */
    auto next() noexcept;

    friend fdio_ns::fdio_completion;

  private:
    event_t::time_resolution timeout;
    event_source_t source;
    std::mutex lock{};
    fdio_ns::fdio_completion* complete{nullptr};

    event_t& event_loop()
    {
        return get_event_loop(ctx);
    }

    void handleEvent() noexcept;
    void handleTimeout() noexcept;
};

class fdio_timeout_exception : public std::runtime_error
{
  public:
    fdio_timeout_exception() : std::runtime_error("Timeout") {}
};

namespace fdio_ns
{

struct fdio_completion
{
    fdio_completion() = delete;
    fdio_completion(const fdio_completion&) = delete;
    fdio_completion& operator=(const fdio_completion&) = delete;
    fdio_completion(fdio_completion&&) = delete;

    explicit fdio_completion(fdio& fdioInstance) noexcept :
        fdioInstance(fdioInstance) {};
    ~fdio_completion();

    friend fdio;

    void start() noexcept;

  private:
    virtual void complete() noexcept = 0;
    virtual void error(std::exception_ptr exceptionPtr) noexcept = 0;
    virtual void stop() noexcept = 0;

    fdio& fdioInstance;
    event_source_t source;
};

// Implementation (templated based on Receiver) of fdio_completion.
template <execution::receiver Receiver>
struct fdio_operation : fdio_completion
{
    fdio_operation(fdio& fdioInstance, Receiver r) :
        fdio_completion(fdioInstance), receiver(std::move(r))
    {}

  private:
    void complete() noexcept override final
    {
        execution::set_value(std::move(receiver));
    }

    void error(std::exception_ptr exceptionPtr) noexcept override final
    {
        execution::set_error(std::move(receiver), exceptionPtr);
    }

    void stop() noexcept override final
    {
        execution::set_stopped(std::move(receiver));
    }

    Receiver receiver;
};

// fdio Sender implementation.
struct fdio_sender
{
    using sender_concept = execution::sender_t;

    fdio_sender() = delete;
    explicit fdio_sender(fdio& fdioInstance) noexcept :
        fdioInstance(fdioInstance) {};

    template <typename Self, class... Env>
    static constexpr auto get_completion_signatures(Self&&, Env&&...)
        -> execution::completion_signatures<
            execution::set_value_t(),
            execution::set_error_t(std::exception_ptr),
            execution::set_stopped_t()>;

    template <execution::receiver R>
    auto connect(R r) -> fdio_operation<R>
    {
        return {fdioInstance, std::move(r)};
    }

  private:
    fdio& fdioInstance;
};

} // namespace fdio_ns

inline auto fdio::next() noexcept
{
    return fdio_ns::fdio_sender{*this};
}

} // namespace sdbusplus::async
