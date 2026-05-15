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

// Move-only handle for completing a D-Bus method reply asynchronously,
// without a coroutine.  See the "Deferred-completion methods" section of
// docs/asio.md for usage.
class deferred_reply
{
  public:
    explicit deferred_reply(message_t&& request) : request_(std::move(request))
    {}

    deferred_reply(const deferred_reply&) = delete;
    deferred_reply& operator=(const deferred_reply&) = delete;

    // Moving transfers ownership of request_.  A moved-from object holds a
    // null request_, so its send() calls become no-ops and its destruction
    // sends nothing.
    deferred_reply(deferred_reply&&) noexcept = default;
    deferred_reply& operator=(deferred_reply&&) noexcept = default;

    ~deferred_reply() = default;

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
     *  `reply.message().new_method_errno(EIO).method_return()`.  As with
     *  send(), the call must be completed exactly once; do not also call
     *  send() on the same deferred_reply.
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
class deferred_callback_method_instance
{
  private:
    using ActualSignature = boost::callable_traits::args_t<CallbackType>;
    using DecayedSignature = utility::decay_tuple_t<ActualSignature>;
    using DbusTupleType = utility::strip_first_arg_t<DecayedSignature>;

    static_assert(
        std::is_same_v<utility::get_first_arg_t<DecayedSignature>,
                       deferred_reply>,
        "deferred_callback_method_instance: handler's first parameter "
        "must be sdbusplus::asio::deferred_reply");

    CallbackType func_;

  public:
    explicit deferred_callback_method_instance(CallbackType&& func) :
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

        // Transfer ownership of the request into the deferred reply, reusing
        // its existing reference rather than taking a fresh sd_bus_message_ref.
        // Safe because method_handler returns as soon as we return 1 and never
        // touches the message again.
        deferred_reply reply{std::move(m)};
        std::apply(
            [this, &reply](auto&&... args) {
                func_(std::move(reply), std::forward<decltype(args)>(args)...);
            },
            dbusArgs);

        return 1;
    }
};

} // namespace asio
} // namespace sdbusplus
