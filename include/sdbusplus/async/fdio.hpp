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

    /** Get the next fd event with a timeout.
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `timed_next()`.
     */
    auto timed_next() noexcept;

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

    friend void tag_invoke(execution::start_t, fdio_completion& self) noexcept
    {
        self.arm();
    }

  private:
    virtual void complete(bool timedOut) noexcept = 0;
    virtual void error(std::exception_ptr exceptionPtr) noexcept = 0;
    virtual void stop() noexcept = 0;
    void arm() noexcept;

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
    void complete(bool) noexcept override final
    {
        execution::set_value(std::move(receiver));
    }

    void error(std::exception_ptr) noexcept override final {}

    void stop() noexcept override final
    {
        execution::set_stopped(std::move(receiver));
    }

    Receiver receiver;
};

// fdio Sender implementation.
struct fdio_sender
{
    using is_sender = void;

    fdio_sender() = delete;
    explicit fdio_sender(fdio& fdioInstance) noexcept :
        fdioInstance(fdioInstance) {};

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const fdio_sender&, auto)
        -> execution::completion_signatures<execution::set_value_t(),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, fdio_sender&& self, R r)
        -> fdio_operation<R>
    {
        return {self.fdioInstance, std::move(r)};
    }

  private:
    fdio& fdioInstance;
};

// Implementation (templated based on Receiver) of fdio_completion for timed
// operations.
template <execution::receiver Receiver>
struct fdio_timed_operation : fdio_completion
{
    fdio_timed_operation(fdio& fdioInstance, Receiver r) :
        fdio_completion(fdioInstance), receiver(std::move(r))
    {}

  private:
    void complete(bool timedOut) noexcept override final
    {
        if (timedOut)
        {
            execution::set_value(std::move(receiver), false);
            return;
        }

        execution::set_value(std::move(receiver), true);
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

// fdio timed Sender implementation.
struct fdio_timed_sender
{
    using is_sender = void;

    fdio_timed_sender() = delete;
    explicit fdio_timed_sender(fdio& fdioInstance) noexcept :
        fdioInstance(fdioInstance) {};

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const fdio_timed_sender&, auto)
        -> execution::completion_signatures<
            execution::set_value_t(bool),
            execution::set_error_t(std::exception_ptr),
            execution::set_stopped_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, fdio_timed_sender&& self, R r)
        -> fdio_timed_operation<R>
    {
        return {self.fdioInstance, std::move(r)};
    }

  private:
    fdio& fdioInstance;
};

} // namespace fdio_ns

inline auto fdio::next() noexcept
{
    return fdio_ns::fdio_sender{*this};
}

inline auto fdio::timed_next() noexcept
{
    return fdio_ns::fdio_timed_sender{*this};
}

} // namespace sdbusplus::async
