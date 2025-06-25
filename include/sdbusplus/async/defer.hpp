#pragma once

#include <sdbusplus/async/context.hpp>
#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/event.hpp>

namespace sdbusplus::async
{

namespace defer_ns
{
struct defer_completion;
}

class defer : private context_ref, details::context_friend
{
  public:
    defer() = delete;
    defer(const defer&) = delete;
    defer& operator=(const defer&) = delete;
    defer(defer&&) = delete;
    defer& operator=(defer&&) = delete;
    ~defer() = default;

    /** Construct a new defer object from a context. */
    explicit defer(context& ctx);

    /** Get the next deferred task.
     *  Note: the implementation only supports a single awaiting task.  Two
     *  tasks should not share this object and both call `next`.
     */
    auto next() noexcept;

    friend defer_ns::defer_completion;

  private:
    event_source_t source;
    std::mutex lock{};
    bool pending{false};
    defer_ns::defer_completion* complete{nullptr};

    event_t& event_loop()
    {
        return get_event_loop(ctx);
    }

    void handleEvent() noexcept;
};

namespace defer_ns
{

struct defer_completion
{
    defer_completion() = delete;
    defer_completion(const defer_completion&) = delete;
    defer_completion& operator=(const defer_completion&) = delete;
    defer_completion(defer_completion&&) = delete;

    explicit defer_completion(defer& deferInstance) noexcept :
        deferInstance(deferInstance) {};
    ~defer_completion();

    friend defer;

    friend void tag_invoke(execution::start_t, defer_completion& self) noexcept
    {
        self.arm();
    }

  private:
    virtual void complete() noexcept = 0;
    virtual void stop() noexcept = 0;
    void arm() noexcept;

    defer& deferInstance;
};

// Implementation (templated based on Receiver) of defer_completion.
template <execution::receiver Receiver>
struct defer_operation : defer_completion
{
    defer_operation(defer& deferInstance, Receiver r) :
        defer_completion(deferInstance), receiver(std::move(r))
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

// defer Sender implementation.
struct defer_sender
{
    using is_sender = void;

    defer_sender() = delete;
    explicit defer_sender(defer& deferInstance) noexcept :
        deferInstance(deferInstance) {};

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const defer_sender&, auto)
        -> execution::completion_signatures<execution::set_value_t(),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, defer_sender&& self, R r)
        -> defer_operation<R>
    {
        return {self.deferInstance, std::move(r)};
    }

  private:
    defer& deferInstance;
};

} // namespace defer_ns

inline auto defer::next() noexcept
{
    return defer_ns::defer_sender{*this};
}

} // namespace sdbusplus::async
