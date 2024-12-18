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

    /** Construct a new fdio object from a context and a file descriptor. */
    fdio(context& ctx, int fd);

    /** Get the next fd event.
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `next`.
     */
    auto next() noexcept;

    friend fdio_ns::fdio_completion;

  private:
    event_source_t source;
    std::mutex lock{};
    fdio_ns::fdio_completion* complete{nullptr};

    event_t& event_loop()
    {
        return get_event_loop(ctx);
    }

    void handleEvent() noexcept;
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
    virtual void complete() noexcept = 0;
    virtual void stop() noexcept = 0;
    void arm() noexcept;

    fdio& fdioInstance;
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

} // namespace fdio_ns

inline auto fdio::next() noexcept
{
    return fdio_ns::fdio_sender{*this};
}

} // namespace sdbusplus::async
