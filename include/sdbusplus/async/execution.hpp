#pragma once

/* This file is intended to be an indirection to libunifex so that it can
 * eventually be replaced with P0443 from C++23.  Users of sdbusplus should
 * refrain from directly including any unifex header file and instead should
 * use these indirections.
 */

#include <unifex/async_scope.hpp>
#include <unifex/defer.hpp>
#include <unifex/done_as_optional.hpp>
#include <unifex/receiver_concepts.hpp>
#include <unifex/scheduler_concepts.hpp>
#include <unifex/sender_concepts.hpp>
#include <unifex/task.hpp>
#include <unifex/timed_single_thread_context.hpp>

#include <coroutine>
#include <exception>

namespace sdbusplus::execution
{
using unifex::async_scope;
using unifex::defer;
using unifex::done_as_optional;
using unifex::receiver_of;
using unifex::schedule;
using unifex::sender;
using unifex::set_done;
using unifex::set_error;
using unifex::set_value;
using unifex::task;
using unifex::timed_single_thread_context;

/** Define a class as a `Sender`.
 *
 *  In order for a class to be considered a 'sender' it must provide a few
 *  types and constants.  Inherit from this template to do so.
 *
 *  @tparam SendsDone - Does the sender indicate `set_done`?
 *  @tparam Results - The result types of the sender.
 */
template <bool SendsDone, typename... Results>
struct sender_of
{
    template <template <typename...> class Variant,
              template <typename...> class Tuple>
    using value_types = Variant<Tuple<Results...>>;

    template <template <typename...> class Variant>
    using error_types = Variant<std::exception_ptr>;

    static constexpr bool sends_done = SendsDone;
};

/** Require Callable is a function-like (lambda, std::function, etc.) type
 *  that returns a sender (or task).
 */
template <typename Callable>
concept returns_sender = execution::sender<std::invoke_result_t<Callable>>;

} // namespace sdbusplus::execution
