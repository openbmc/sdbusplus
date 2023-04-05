#pragma once

#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/exception.hpp>

#include <deque>
#include <mutex>

namespace sdbusplus::async
{

namespace scope_ns
{
template <execution::sender Sender>
struct scope_receiver;

struct scope_sender;

struct scope_completion;
} // namespace scope_ns

/** A collection of tasks.
 *
 *  It is sometimes useful to run a group of tasks, possibly even detached
 *  from an execution context, but the only option in std::execution is
 *  start_detached, which does not have any mechanism to deal with errors
 *  from those tasks and can potentially leak tasks.  Alternatively, you
 *  can save the result of `execution::connect`, but you need to maintain a
 *  lifetime of it until the connected-task completes.
 *
 *  The scope maintains the lifetime of the tasks it `spawns` so that the
 *  operational-state created by `connect` is preserved (and deleted) as
 *  appropriate.
 */
struct scope
{
    scope() = delete;
    explicit scope(execution::run_loop& loop) : loop(loop) {}

    // The scope destructor can throw if it was destructed while there are
    // outstanding tasks.
    ~scope() noexcept(false);

    /** Spawn a Sender to run on the scope.
     *
     *  @param[in] sender - The sender to run.
     */
    template <execution::sender_of<execution::set_value_t()> Sender>
    void spawn(Sender&& sender);

    /** Get a Sender that awaits for all tasks to complete. */
    scope_ns::scope_sender empty() noexcept;

    template <execution::sender>
    friend struct scope_ns::scope_receiver;

    friend scope_ns::scope_completion;

  private:
    void started_task() noexcept;
    void ended_task(std::exception_ptr&&) noexcept;

    std::mutex lock{};
    bool started = false;
    size_t pending_count = 0;
    std::deque<std::exception_ptr> pending_exceptions = {};
    scope_ns::scope_completion* pending = nullptr;

    execution::run_loop& loop;
};

namespace scope_ns
{

/** The Sender-completion Reciever. */
template <execution::sender Sender>
struct scope_receiver
{
    template <typename OpState>
    explicit scope_receiver(OpState* os, scope* s) : op_state(os), s(s)
    {}

    friend void tag_invoke(execution::set_value_t, scope_receiver&& self,
                           auto&&...) noexcept
    {
        self.complete();
    }

    friend void tag_invoke(execution::set_error_t, scope_receiver&& self,
                           std::exception_ptr&& e) noexcept
    {
        self.complete(std::move(e));
    }

    friend void tag_invoke(execution::set_stopped_t,
                           scope_receiver&& self) noexcept
    {
        self.complete(std::make_exception_ptr(exception::UnhandledStop{}));
    }

    friend decltype(auto) tag_invoke(execution::get_env_t,
                                     const scope_receiver& self) noexcept
    {
        return self;
    }

    friend decltype(auto) tag_invoke(execution::get_scheduler_t,
                                     const scope_receiver& self) noexcept
    {
        return self.get_scheduler();
    }

    void complete(std::exception_ptr&& = {}) noexcept;

    void* op_state;
    scope* s = nullptr;

  private:
    decltype(auto) get_scheduler() const
    {
        return s->loop.get_scheduler();
    }
};

/** The holder of the connect operational-state. */
template <execution::sender Sender>
struct scope_operation_state
{
    using state_t = execution::connect_result_t<Sender, scope_receiver<Sender>>;
    std::optional<state_t> op_state;
};

template <execution::sender Sender>
void scope_receiver<Sender>::complete(std::exception_ptr&& e) noexcept
{
    // The Sender is complete, so we need to clean up the saved operational
    // state.

    // Save the scope (since we're going to delete `this`).
    auto owning_scope = s;

    // Delete the operational state, which triggers deleting this.
    delete static_cast<scope_ns::scope_operation_state<Sender>*>(op_state);

    // Inform the scope that a task has completed.
    owning_scope->ended_task(std::move(e));
}

// Virtual class to handle the scope completions.
struct scope_completion
{
    scope_completion() = delete;
    scope_completion(scope_completion&&) = delete;

    explicit scope_completion(scope& s) : s(s){};
    virtual ~scope_completion() = default;

    friend scope;

    friend void tag_invoke(execution::start_t, scope_completion& self) noexcept
    {
        self.arm();
    }

  private:
    virtual void complete(std::exception_ptr&&) noexcept = 0;
    void arm() noexcept;

    scope& s;
};

// Implementation (templated based on Reciever) of scope_completion.
template <execution::receiver Reciever>
struct scope_operation : scope_completion
{
    scope_operation(scope& s, Reciever r) :
        scope_completion(s), receiver(std::move(r))
    {}

  private:
    void complete(std::exception_ptr&& e) noexcept override final
    {
        if (e)
        {
            execution::set_error(std::move(receiver), std::move(e));
        }
        else
        {
            execution::set_value(std::move(receiver));
        }
    }

    Reciever receiver;
};

// Scope completion Sender implementation.
struct scope_sender
{
    scope_sender() = delete;
    explicit scope_sender(scope& m) noexcept : m(m){};

    friend auto tag_invoke(execution::get_completion_signatures_t,
                           const scope_sender&, auto)
        -> execution::completion_signatures<execution::set_value_t(),
                                            execution::set_stopped_t()>;

    template <execution::receiver R>
    friend auto tag_invoke(execution::connect_t, scope_sender&& self, R r)
        -> scope_operation<R>
    {
        return {self.m, std::move(r)};
    }

  private:
    scope& m;
};

// Most (non-movable) receivers cannot be emplaced without this template magic.
// Ex. `spawn(std::execution::just())` doesnt' work without this.
template <typename Fn>
struct in_place_construct
{
    Fn fn;
    using result_t = decltype(std::declval<Fn>()());

    operator result_t()
    {
        return ((Fn &&) fn)();
    }
};

} // namespace scope_ns

template <execution::sender_of<execution::set_value_t()> Sender>
void scope::spawn(Sender&& sender)
{
    // Create a holder of the operational state.  Keep it in a unique_ptr
    // so it is cleaned up if there are any exceptions in this function.
    auto s = std::make_unique<
        scope_ns::scope_operation_state<std::decay_t<Sender>>>();

    // Associate the state and scope with the receiver and connect to the
    // Sender.
    s->op_state.emplace(scope_ns::in_place_construct{[&] {
        return execution::connect(
            std::forward<Sender>(sender),
            scope_ns::scope_receiver<std::decay_t<Sender>>{s.get(), this});
    }});

    started_task();

    // Start is noexcept, so it is safe to release the pointer which is now
    // contained in the receiver.
    execution::start(s.release()->op_state.value());
}

} // namespace sdbusplus::async
