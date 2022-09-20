#pragma once

#include <sdbusplus/async/execution.hpp>

#include <atomic>

namespace sdbusplus::async
{

namespace scope_ns
{
template <execution::sender Sender>
struct scope_receiver;
}

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
    scope() = default;

    // The scope destructor can throw if it was destructed while there are
    // outstanding tasks.
    ~scope() noexcept(false);

    /** Spawn a Sender to run on the scope.
     *
     *  @param[in] sender - The sender to run.
     */
    template <execution::sender Sender>
    void spawn(Sender&& sender);

    template <execution::sender>
    friend struct scope_ns::scope_receiver;

  private:
    void started_task() noexcept;
    void ended_task() noexcept;

    std::atomic<size_t> count = 0;
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
                           auto&&) noexcept
    {
        std::terminate(); // TODO: save the exception back into the scope.
        self.complete();
    }

    friend void tag_invoke(execution::set_stopped_t,
                           scope_receiver&& self) noexcept
    {
        self.complete();
    }

    friend decltype(auto) tag_invoke(execution::get_env_t,
                                     const scope_receiver& self) noexcept
    {
        return self;
    }

    void complete() noexcept;

    void* op_state;
    scope* s = nullptr;
};

/** The holder of the connect operational-state. */
template <execution::sender Sender>
struct scope_operation_state
{
    using state_t = execution::connect_result_t<Sender, scope_receiver<Sender>>;
    std::optional<state_t> op_state;
};

template <execution::sender Sender>
void scope_receiver<Sender>::complete() noexcept
{
    // The Sender is complete, so we need to clean up the saved operational
    // state.

    // Save the scope (since we're going to delete `this`).
    auto owning_scope = s;

    // Delete the operational state, which triggers deleting this.
    delete static_cast<scope_ns::scope_operation_state<Sender>*>(op_state);

    // Inform the scope that a task has completed.
    owning_scope->ended_task();
}

} // namespace scope_ns

template <execution::sender Sender>
void scope::spawn(Sender&& sender)
{
    // Create a holder of the operational state.  Keep it in a unique_ptr
    // so it is cleaned up if there are any exceptions in this function.
    auto s = std::make_unique<
        scope_ns::scope_operation_state<std::decay_t<Sender>>>();

    // Associate the state and scope with the receiver and connect to the
    // Sender.
    s->op_state.emplace(execution::connect(
        std::forward<Sender>(sender),
        scope_ns::scope_receiver<std::decay_t<Sender>>{s.get(), this}));

    started_task();

    // Start is noexcept, so it is safe to release the pointer which is now
    // contained in the receiver.
    execution::start(s.release()->op_state.value());
}

} // namespace sdbusplus::async
