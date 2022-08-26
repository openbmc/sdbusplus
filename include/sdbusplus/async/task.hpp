#pragma once

#include <sdbusplus/async/execution.hpp>

#include <exception>
#include <type_traits>
#include <variant>

namespace sdbusplus::async
{
// Forward declare the internal task type.
namespace task_ns
{
template <typename T>
class task;
}

/** @brief The `task` implementation.
 *
 *  This is an implementation of the 'task' concept from the coroutines
 *  and executors papers.  Generally, developers shouldn't care about the
 *  implementation of this.  A traditional function can be transformed into
 *  a coroutine with
 *
 *  ```
 *      int the_answer() { return 42; }
 *      task<int> co_the_answer() { co_return 42; }
 *  ```
 *
 *  and now instead of `auto result = the_answer()`, the result can be obtained
 *  as a coroutine with `auto result = co_await co_the_answer()`.
 *
 *  It may be useful to read the current coroutines documentation and the
 *  proposed C++26 executors paper if you are interested in more details.
 *
 *  https://en.cppreference.com/w/cpp/language/coroutines
 *  https://brycelelbach.github.io/wg21_p2300_std_execution/std_execution.html
 *
 *  As an aside, `co_await co_the_answer()` is equivalent to
 *  `co_await std::execution::just(42)` from the executors proposal, as senders
 *  and co-routines are [usually] interchangeable.
 *
 *  Most of this file is simply ugly template boiler-plate required to make it
 *  work.  I've used presentations and blog posts from some of the executors
 *  authors and reference implementations from p2300, libunifex, and cppcoro
 *  to create a "simple as I can make it" task implementation.
 *
 *  Co-routine tasks generally consist of 3 parts:
 *      - The promise.
 *      - The awaitable.
 *      - The task.
 *
 *  The promise is a handle to the result that will eventually come from the
 *  co-routine.  When a co-routine completes, the promise is fulfilled and
 *  the value that was `co_return` can come out at the `co_await` location.
 *
 *  The awaitable the context that allows a co-routine to be suspended and
 *  resumed (at the `co_await` location).  This typically is 'suspended' to
 *  begin with and 'continued' when the promise is fulfilled.
 *
 *  The task could be described as the handle to a "awaitable generator".  Via
 *  the 'co_await' operator, a task is transformed into an awaitable.  Up until
 *  the point this transformation happens, the task can be destructed having
 *  accomplished no side-effects.  Consider the following code:
 *
 *  ```
 *  auto foo() -> task<>
 *  {
 *     std::cout << "Hello from foo!\n";
 *
 *     auto answer = co_await std::execution::just(42);
 *     std::cout << "The answer is " << answer << std::endl;
 *
 *     co_return;
 *  }
 *
 *  task<> f = foo();
 *  if (condition)
 *  {
 *      co_await std::move(f);
 *  }
 *  ```
 *
 *  All the `std::cout` calls are only executed if the `condition` is met, but
 *  otherwise `f` can be destructed without any effect.  Even the `cout` which
 *  occurs before the `co_await` within `foo` is never executed.
 *
 *  Within the executors proposal there is support for "stop tokens", which I
 *  have not fully grasped the utility of yet.  For now I've kept the task
 *  implementation not supporting stop tokens [which is much simpler].
 */
template <typename T = void>
using task = task_ns::task<T>;

namespace task_ns
{

/** Enum to track how the promise completed. */
enum class completed_as
{
    not_completed = 0,
    exception = 1,
    value = 2,
};

/** The promise data holder for a non-void type. */
template <typename T>
struct promise_base
{
    std::variant<std::monostate, std::exception_ptr, T> data;

    void return_value(T const& value) noexcept(
        std::is_nothrow_constructible_v<T, T const&>)
    {
        data.template emplace<(size_t)completed_as::value>(value);
    }

    void return_value(T&& value) noexcept(
        std::is_nothrow_constructible_v<T, T&&>)
    {
        data.template emplace<(size_t)completed_as::value>(std::move(value));
    }

    using completion_signatures_t =
        std::execution::completion_signatures<std::execution::set_value_t(T),
                                              std::execution::set_error_t(
                                                  std::exception_ptr)>;
};

/** The promise data holder for a void type. */
template <>
struct promise_base<void>
{
    // Make up a struct to avoid collisions with any other type.
    struct placeholder_t
    {};
    std::variant<std::monostate, std::exception_ptr, placeholder_t> data;

    void return_void() noexcept
    {
        data.template emplace<(size_t)completed_as::value>();
    }

    using completion_signatures_t =
        std::execution::completion_signatures<std::execution::set_value_t(),
                                              std::execution::set_error_t(
                                                  std::exception_ptr)>;
};

template <typename T>
struct final_awaitable;

/** The boiler-plate for the common parts of the promise (all the parts that
 *  don't revolve around holding data).
 */
template <typename T>
struct promise :
    promise_base<T>,
    std::execution::with_awaitable_senders<promise<T>>
{
    task<T> get_return_object() noexcept
    {
        return task(std::coroutine_handle<promise>::from_promise(*this));
    }

    auto initial_suspend() noexcept
    {
        return std::suspend_always{};
    }

    auto final_suspend() noexcept
    {
        return final_awaitable<T>{};
    }

    void unhandled_exception() noexcept
    {
        this->data.template emplace<(size_t)completed_as::exception>(
            std::current_exception());
    }

    friend auto tag_invoke(std::execution::get_env_t, const promise<T>& self)
    {
        return self;
    }
};

/** @brief Used to do the final continuation to the caller's coroutine.
 */
template <typename T>
struct final_awaitable
{
    static auto await_ready() noexcept
    {
        return std::false_type{};
    }
    static auto await_suspend(std::coroutine_handle<promise<T>> handle) noexcept
    {
        return handle.promise().continuation();
    }
    static void await_resume() noexcept {}
};

/** @brief The awaitable for a task. */
template <typename T>
struct task_awaitable
{
    std::coroutine_handle<promise<T>> handle;

    static auto await_ready() noexcept
    {
        return std::false_type{};
    }

    template <typename P>
    auto await_suspend(std::coroutine_handle<P> handle2) noexcept
    {
        handle.promise().set_continuation(handle2);
        return std::coroutine_handle<>(handle);
    }

    T await_resume()
    {
        // No matter what we do we are done with the handle.  Destroy
        // it on exit.
        struct handle_destroy
        {
            ~handle_destroy()
            {
                std::exchange(t->handle, {}).destroy();
            }
            task_awaitable* t;

        } destroyer{this};

        // If the promise completed as exception, rethrow it.
        if (handle.promise().data.index() == (size_t)completed_as::exception)
        {
            std::rethrow_exception(std::get<(size_t)completed_as::exception>(
                std::move(handle.promise().data)));
        }
        // Otherwise, return the value.
        if constexpr (!std::is_void_v<T>)
        {
            return std::get<(size_t)completed_as::value>(
                std::move(handle.promise().data));
        }
    }
};

/** @brief The task class. */
template <typename T>
class task
{
  public:
    using promise_type = promise<T>;
    using handle_type = std::coroutine_handle<promise_type>;

    task(task&& other) noexcept : handle(std::exchange(other.handle, {})) {}

    ~task()
    {
        if (handle)
        {
            handle.destroy();
        }
    }

    explicit task(handle_type h) noexcept : handle(h) {}

    template <typename S>
    friend task_awaitable<T> operator co_await(task<S>&& self) noexcept
    {
        return task_awaitable<T>{std::exchange(self.handle, {})};
    }

    friend auto tag_invoke(std::execution::get_completion_signatures_t,
                           const task&, auto) ->
        typename promise_type::completion_signatures_t;

  private:
    handle_type handle;
};

} // namespace task_ns

} // namespace sdbusplus::async
