#pragma once

#include <sdbusplus/async/callback.hpp>
#include <sdbusplus/async/context.hpp>
#include <sdbusplus/message.hpp>

#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <variant>

namespace sdbusplus::async
{

/** A (client-side) proxy to a dbus object.
 *
 *  A dbus object is referenced by 3 address pieces:
 *      - The service hosting the object.
 *      - The path the object resides at.
 *      - The interface the object implements.
 *  The proxy is a holder of these 3 addresses.
 *
 *  One all 3 pieces of the address are known by the proxy, the proxy
 *  can be used to perform normal dbus operations: method-call, get-property
 *  set-property, get-all-properties.
 *
 *  Addresses are supplied to the object by calling the appropriate method:
 *  service, path, interface.  These methods return a _new_ object with the
 *  new information filled in.
 *
 *  If all pieces are known at compile-time it is suggested to be constructed
 *  similar to the following:
 *
 *  ```
 *  constexpr auto systemd = sdbusplus::async::proxy()
 *                              .service("org.freedesktop.systemd1")
 *                              .path("/org/freedesktop/systemd1")
 *                              .interface("org.freedesktop.systemd1.Manager");
 *  ```
 *
 *  The proxy object can be filled as information is available and attempts
 *  to be as effecient as possible (supporting constexpr construction and
 *  using std::string_view mostly).  In some cases it is necessary for the
 *  proxy to leave a scope where it would be no longer safe to use the
 *  previously-supplied string_views.  The `preserve` operation can be used
 *  to transform an existing proxy into one which is safe to leave (because
 *  it uses less-efficient but safe std::string values).
 */
template <bool S = false, bool P = false, bool I = false,
          bool Preserved = false>
struct proxy : private sdbusplus::bus::details::bus_friend
{
    // Some typedefs to reduce template noise...

    using string_t =
        std::conditional_t<Preserved, std::string, std::string_view>;
    using string_ref = const string_t&;
    using sv_ref = const std::string_view&;

    template <bool V>
    using value_t = std::conditional_t<V, string_t, std::monostate>;
    template <bool V>
    using value_ref = const value_t<V>&;

    // Default constructor should only work for the "all empty" case.
    constexpr proxy() requires(!S and !P and !I) = default;
    proxy() requires(S or P or I) = delete;

    // Construtor allowing all 3 to be passed in.
    constexpr proxy(value_ref<S> s, value_ref<P> p, value_ref<I> i) :
        s(s), p(p), i(i){};

    // Functions to assign address fields.
    constexpr auto service(string_ref s) const noexcept requires(!S)
    {
        return proxy<true, P, I, Preserved>{s, this->p, this->i};
    }
    constexpr auto path(string_ref p) const noexcept requires(!P)
    {
        return proxy<S, true, I, Preserved>{this->s, p, this->i};
    }
    constexpr auto interface(string_ref i) const noexcept requires(!I)
    {
        return proxy<S, P, true, Preserved>{this->s, this->p, i};
    }

    /** Make a copyable / returnable proxy.
     *
     *  Since proxy deals with string_view by default, for efficiency,
     *  there are cases where it would be dangerous for a proxy object to
     *  leave a scope either by a return or a pass into a lambda.  This
     *  function will convert an existing proxy into one backed by
     *  `std::string` so that it can safely leave a scope.
     */
    auto preserve() const noexcept requires(!Preserved)
    {
        return proxy<S, P, I, true>{std::string{this->s}, std::string{this->p},
                                    std::string{this->i}};
    }

    /** Perform a method call.
     *
     *  @tparam Rs - The return type(s) of the method call.
     *  @tparam Ss - The parameter type(s) of the method call.
     *
     *  @param[in] ctx - The context to use.
     *  @param[in] method - The method name.
     *  @param[in] ss - The calling parameters.
     *
     *  @return A Sender which completes with either { void, Rs, tuple<Rs...> }.
     */
    template <typename... Rs, typename... Ss>
    auto call(context& ctx, sv_ref method, Ss&&... ss) const
        requires(S and P and I)
    {
        // Create the method_call message.
        auto msg = ctx.get_bus().new_method_call(c_str(s), c_str(p), c_str(i),
                                                 method.data());
        if constexpr (sizeof...(Ss) > 0)
        {
            msg.append(std::forward<Ss>(ss)...);
        }

        // Use 'callback' to perform the operation and "then" "unpack" the
        // contents.
        return execution::then(
            callback([bus = get_busp(ctx.get_bus()),
                      msg = std::move(msg)](auto cb, auto data) {
                return sd_bus_call_async(bus, nullptr, msg.get(), cb, data, 0);
            }),
            unpack<Rs...>);
    }

    /** Get a property.
     *
     *  @tparam T - The type of the property.
     *
     *  @param[in] ctx - The context to use.
     *  @param[in] property - The property name.
     *
     *  @return A Sender which completes with T as the property value.
     */
    template <typename T>
    auto get_property(context& ctx, sv_ref property) const
        requires(S and P and I)
    {
        return execution::then(
            proxy(s, p, dbus_prop_intf)
                .call<std::variant<T>>(ctx, "Get", c_str(i), property.data()),
            [](auto&& v) { return std::get<T>(v); });
    }

    /** Get all properties.
     *
     * @tparam V - The variant type of all possible properties.
     *
     * @param[in] ctx - The context to use.
     *
     * @return A Sender which completes with unordered_map<string, V>.
     */
    template <typename V>
    auto get_all_properties(context& ctx) const requires(S and P and I)
    {
        return proxy(s, p, dbus_prop_intf)
            .call<std::unordered_map<std::string, V>>(ctx, "GetAll", c_str(i));
    }

    /** Set a property.
     *
     * @tparam T - The type of the property (usually deduced by the compiler).
     *
     * @param[in] ctx - The context to use.
     * @param[in] property - The property name.
     * @param[in] value - The value to set.
     *
     * @return A Sender which completes void when the property is set.
     */
    template <typename T>
    auto set_property(context& ctx, sv_ref property, T&& value) const
        requires(S and P and I)
    {
        return proxy(s, p, dbus_prop_intf)
            .call<>(ctx, "Set", c_str(i), property.data(),
                    std::forward<T>(value));
    }

  private:
    static constexpr auto dbus_prop_intf = "org.freedesktop.DBus.Properties";

    // Helper to get the underlying c-string of a string_view or string.
    static auto c_str(string_ref v)
    {
        if constexpr (Preserved)
        {
            return v.c_str();
        }
        else
        {
            return v.data();
        }
    }

    // Unpack a message into requested types.
    template <typename... Rs>
    static auto unpack(message_t m)
    {
        if constexpr (sizeof...(Rs) > 1)
        {
            std::tuple<Rs...> r{};
            m.read(std::apply(r));
            return r;
        }
        else if constexpr (sizeof...(Rs) == 1)
        {
            std::tuple_element_t<0, std::tuple<Rs...>> r{};
            m.read(r);
            return r;
        }
        else
        {
            return;
        }
    }

    value_t<S> s = {};
    value_t<P> p = {};
    value_t<I> i = {};
};

} // namespace sdbusplus::async
