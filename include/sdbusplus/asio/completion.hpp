#pragma once

#include <boost/callable_traits/args.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <cerrno>
#include <tuple>
#include <type_traits>
#include <utility>

namespace sdbusplus
{
namespace asio
{

// Move-only completion handle for finishing a D-Bus method call
// asynchronously, without a coroutine.  See the "Completion methods" section
// of docs/asio.md for usage.
class completion
{
  public:
    explicit completion(message_t&& request) : request_(std::move(request)) {}

    completion(const completion&) = delete;
    completion& operator=(const completion&) = delete;

    // Moving transfers ownership of request_.  A moved-from object holds a
    // null request_, so its send() calls become no-ops and its destruction
    // sends nothing.
    completion(completion&&) noexcept = default;
    completion& operator=(completion&&) noexcept = default;

    ~completion() = default;

    /** Send a successful method return, appending args as the reply body.
     *
     *  Idempotent: only the first call transmits a reply.  Subsequent
     *  calls, and calls on a moved-from object, are no-ops.
     */
    template <typename... Args>
    void send(Args&&... args)
    {
        if (replied_ || !request_)
        {
            return;
        }
        auto ret = request_.new_method_return();
        (ret.append(std::forward<Args>(args)), ...);
        ret.method_return();
        replied_ = true;
    }

    /** Access the originating method-call message.
     *
     *  Use this to build and send a custom reply (for example an error)
     *  with the message_t API, e.g.
     *  `c.message().new_method_errno(EIO).method_return()`.  As with
     *  send(), the call must be completed exactly once; do not also call
     *  send() on the same completion.
     */
    message_t& message() noexcept
    {
        return request_;
    }

  private:
    message_t request_;
    bool replied_ = false;
};

template <typename CallbackType>
class completion_method_instance
{
  private:
    using ActualSignature = boost::callable_traits::args_t<CallbackType>;
    using DecayedSignature = utility::decay_tuple_t<ActualSignature>;
    using DbusTupleType = utility::strip_first_arg_t<DecayedSignature>;

    static_assert(
        std::is_same_v<utility::get_first_arg_t<DecayedSignature>, completion>,
        "completion_method_instance: handler's first parameter "
        "must be sdbusplus::asio::completion");

    CallbackType func_;

  public:
    explicit completion_method_instance(CallbackType&& func) :
        func_(std::move(func))
    {}

    int operator()(message_t& m)
    {
        if (m.is_method_error())
        {
            return -EINVAL;
        }

        DbusTupleType dbusArgs;
        std::apply([&m](auto&... x) { m.read(x...); }, dbusArgs);

        // Transfer ownership of the request into the completion, reusing its
        // existing reference rather than taking a fresh sd_bus_message_ref.
        // Safe because method_handler returns as soon as we return 1 and never
        // touches the message again.
        completion c{std::move(m)};
        std::apply(
            [this, &c](auto&&... args) {
                func_(std::move(c), std::forward<decltype(args)>(args)...);
            },
            dbusArgs);

        return 1;
    }
};

} // namespace asio
} // namespace sdbusplus
