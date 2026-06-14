#pragma once

#include <boost/callable_traits/args.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <cerrno>
#include <cstddef>
#include <tuple>
#include <utility>

namespace sdbusplus
{
namespace asio
{

// An asio-style completion handler bound to a deferred D-Bus method call.
//
// A method registered with register_completion_method() receives a completion
// as the last handler argument instead of replying synchronously.  The handler
// hands the completion off to a callback-based asynchronous operation, and the
// reply is sent when the completion is later invoked:
//
//   - on success, call it with a default-constructed error_code followed by the
//     method's return values;
//   - on failure, call it with a non-zero error_code, whose errno is returned
//     to the caller via new_method_errno().
//
// The call signature, void(error_code, Results...), is the ordinary asio
// completion-handler shape, so a completion composes with other asio operations
// and may be passed directly as their completion token.
template <typename... Results>
class completion
{
  public:
    explicit completion(message_t call) : call_(std::move(call)) {}

    void operator()(const boost::system::error_code& ec, Results... results)
    {
        if (ec)
        {
            call_.new_method_errno(ec.value()).method_return();
            return;
        }
        message_t reply = call_.new_method_return();
        reply.append(std::move(results)...);
        reply.method_return();
    }

  private:
    message_t call_;
};

namespace details
{

// Identify a completion and recover the tuple of its result types.
template <typename T>
struct completion_traits : std::false_type
{};

template <typename... Results>
struct completion_traits<completion<Results...>> : std::true_type
{
    using results_tuple = std::tuple<Results...>;
};

// The first N elements of a tuple, as a tuple.
template <typename Tuple, typename Seq>
struct tuple_take_front;

template <typename Tuple, std::size_t... I>
struct tuple_take_front<Tuple, std::index_sequence<I...>>
{
    using type = std::tuple<std::tuple_element_t<I, Tuple>...>;
};

} // namespace details

// Dispatcher for a deferred method whose handler ends in a completion.  It
// decodes the input arguments, builds the completion from a copy of the call
// message, and invokes the handler.  operator() returns a positive value so
// sd-bus sends no automatic reply; the handler owns the completion and sends
// the reply itself once its asynchronous work completes.
template <typename CallbackType>
class completion_method_instance
{
  private:
    using FullTuple =
        utility::decay_tuple_t<boost::callable_traits::args_t<CallbackType>>;
    static constexpr std::size_t argCount = std::tuple_size_v<FullTuple>;
    static_assert(argCount >= 1,
                  "a completion method handler must take a completion");

    using CompletionType = std::tuple_element_t<argCount - 1, FullTuple>;
    static_assert(details::completion_traits<CompletionType>::value,
                  "the last handler argument must be an "
                  "sdbusplus::asio::completion");

    using InputTupleType = typename details::tuple_take_front<
        FullTuple, std::make_index_sequence<argCount - 1>>::type;

    CallbackType func_;

  public:
    explicit completion_method_instance(CallbackType&& func) : func_(func) {}

    int operator()(message_t& m)
    {
        InputTupleType dbusArgs;
        if (m.is_method_error())
        {
            return -EINVAL;
        }
        std::apply([&m](auto&... x) { m.read(x...); }, dbusArgs);

        // Copy the call message into the completion; the handler keeps it and
        // sends the reply once its asynchronous work completes.
        auto args = std::tuple_cat(
            std::move(dbusArgs), std::make_tuple(CompletionType{message_t{m}}));
        std::apply(func_, std::move(args));
        return 1;
    }
};

} // namespace asio
} // namespace sdbusplus
