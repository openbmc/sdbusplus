#pragma once
#include <function2/function2.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/server.hpp>

#include <type_traits>

/** This file contains support functions for sdbus++-generated server
 *  bindings, which result in space savings by having common definitions.
 */

namespace sdbusplus
{
namespace sdbuspp
{
namespace detail
{

template <typename>
struct Func;

template <typename R, typename... As>
struct Func<R(As&&...)>
{
    using r = R;
    using as = std::tuple<As...>;
};

template <typename... T>
inline void tupleRead(message_t& m, std::tuple<T...>& t)
{
    std::apply([&](T&... t) { (m.read(t), ...); }, t);
}

} // namespace detail

/** Handle common parts of a get/set property callback.
 *
 *  @param[in] msg The message to read to / write from.
 *  @param[in] intf The SdBusInterface type.
 *  @param[in] error The error object for any errors.
 *  @param[in] f The interface function for the get/set.
 *
 *  This function will unpack a message's contents, redirect them to the
 *  function 'f', and then pack them into the response message.
 */
template <typename F>
int property_callback(sd_bus_message* msg, sdbusplus::SdBusInterface* intf,
                      sd_bus_error* error, fu2::function_view<F> f)
{
    try
    {
        // Refcount the message.
        auto m = message_t(msg, intf);

        // Set up the transaction.
        server::transaction::set_id(m);

        // Read arguments from the message.
        typename detail::Func<F>::as arg{};
        detail::tupleRead(m, arg);

        // Call the function with the arguments.
        if constexpr (!std::is_same_v<void, typename detail::Func<F>::r>)
        {
            // Pack results back into message.
            m.append(std::apply(f, std::move(arg)));
        }
        else
        {
            // No return, just direct call.
            std::apply(f, std::move(arg));
        }
    }
    catch (const sdbusplus::internal_exception_t& e)
    {
        return intf->sd_bus_error_set(error, e.name(), e.description());
    }

    // A positive integer must be returned to indicate that the callback
    // has done its work (and not delegate to a later callback in the vtable).
    return 1;
}

/** Handle common parts of a method callback.
 *
 *  @tparam multi_return Set to true if the function returns multiple results.
 *                       Otherwise, this function is unable to differentiate
 *                       between a tuple that should be returned as a tuple or
 *                       a tuple that contains the multiple results and should
 *                       be returned without the tuple wrapper.
 *
 *  @param[in] msg The message to read to / write from.
 *  @param[in] intf The SdBusInterface type.
 *  @param[in] error The error object for any errors.
 *  @param[in] f The interface function for the get/set.
 *
 *  @return a negative return-code on error.
 *
 *  This function will unpack a message's contents, redirect them to the
 *  function 'f', and then pack them into the response message.
 */
template <typename F, bool multi_return = false>
int method_callback(sd_bus_message* msg, sdbusplus::SdBusInterface* intf,
                    sd_bus_error* error, fu2::function_view<F> f)
{
    try
    {
        // Refcount the message.
        auto m = message_t(msg, intf);

        // Set up the transaction.
        server::transaction::set_id(m);

        // Read arguments from the message.
        typename detail::Func<F>::as arg{};
        detail::tupleRead(m, arg);

        // Get the reply message.
        auto reply = m.new_method_return();

        // Call the function with the arguments.
        if constexpr (std::is_same_v<void, typename detail::Func<F>::r>)
        {
            // No return value, direct call.
            std::apply(f, std::move(arg));
        }
        else if constexpr (!multi_return)
        {
            // Single return value; call and append to reply message.
            reply.append(std::apply(f, std::move(arg)));
        }
        else
        {
            // Multi return values; call and append to reply message.
            // Step 1: call 'f' with args from message.
            //      - std::apply(f, std::move(arg))
            // Step 2: append each return from f into the reply message one
            //         at a time.
            //      - Apply on return from 'f' into lambda that does an append.
            std::apply([&](auto&&... v) { (reply.append(std::move(v)), ...); },
                       std::apply(f, std::move(arg)));
        }

        // Indicate reply complete.
        reply.method_return();
    }
    catch (const sdbusplus::internal_exception_t& e)
    {
        return intf->sd_bus_error_set(error, e.name(), e.description());
    }

    // A positive integer must be returned to indicate that the callback
    // has done its work (and not delegate to a later callback in the vtable).
    return 1;
}

} // namespace sdbuspp
} // namespace sdbusplus
