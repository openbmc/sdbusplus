#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/sdbus.hpp>

#include <type_traits>

namespace sdbusplus
{

namespace server
{

namespace object
{

// Forward declaration.
template <class... Args>
struct object;

namespace details
{

// Forward declaration.
template <class... Args>
struct compose;

/** Template to identify if an inheritance is a "normal" type or a nested
 *  sdbusplus::object.
 */
template <class T>
struct compose_inherit
{
    // This is a normal type.
    using type = T;

    // Normal types need emit_added called, if possible.
    template <class S>
    static void maybe_emit_iface_added(S* obj)
    {
        if constexpr (has_emit_added<S>())
        {
            obj->S::emit_added();
        }
    }

    // Test if emit_added() exists in T return std::true_type.
    template <class S>
    static constexpr auto has_emit_added_helper(int)
        -> decltype(std::declval<S>().emit_added(), std::true_type{});

    // If the above test fails, fall back to this to return std::false_type
    template <class>
    static constexpr std::false_type has_emit_added_helper(...);

    // Invoke the test with an int so it first resolves to
    // has_emit_added_helper(int), and when it fails, it resovles to
    // has_emit_added_helper(...) thanks to SFINAE.
    // So the return type is std::true_type if emit_added() exists in T and
    // std::false_type otherwise.
    template <class S>
    using has_emit_added = decltype(has_emit_added_helper<S>(0));
};

/** Template specialization for the sdbusplus::object nesting. */
template <class... Args>
struct compose_inherit<object<Args...>>
{
    // Unravel the inheritance with a recursive compose.
    using type = compose<Args...>;

    // Redirect the interface added to the composed parts.
    template <class T>
    static void maybe_emit_iface_added(T* obj)
    {
        obj->compose<Args...>::maybe_emit_iface_added();
    }
};

/** std-style _t alias for compose_inherit. */
template <class... Args>
using compose_inherit_t = typename compose_inherit<Args...>::type;

/** Templates to allow multiple inheritance via template parameters.
 *
 *  These allow an object to group multiple dbus interface bindings into a
 *  single class.
 */
template <class T, class... Rest>
struct compose_impl : compose_inherit_t<T>, compose_impl<Rest...>
{
    compose_impl(bus_t& bus, const char* path) :
        T(bus, path), compose_impl<Rest...>(bus, path)
    {}

    void maybe_emit_iface_added()
    {
        compose_inherit<T>::maybe_emit_iface_added(
            static_cast<compose_inherit_t<T>*>(this));
        compose_impl<Rest...>::maybe_emit_iface_added();
    }
};

/** Specialization for single element. */
template <class T>
struct compose_impl<T> : compose_inherit_t<T>
{
    compose_impl(bus_t& bus, const char* path) : compose_inherit_t<T>(bus, path)
    {}

    void maybe_emit_iface_added()
    {
        compose_inherit<T>::maybe_emit_iface_added(
            static_cast<compose_inherit_t<T>*>(this));
    }
};

/** Default compose operation for variadic arguments. */
template <class... Args>
struct compose : compose_impl<Args...>
{
    compose(bus_t& bus, const char* path) : compose_impl<Args...>(bus, path) {}

    friend struct compose_inherit<object<Args...>>;

  protected:
    void maybe_emit_iface_added()
    {
        compose_impl<Args...>::maybe_emit_iface_added();
    };
};

/** Specialization for zero variadic arguments. */
template <>
struct compose<>
{
    compose(bus_t& /*bus*/, const char* /*path*/) {}

  protected:
    void maybe_emit_iface_added(){};
};

} // namespace details

/** Class to compose multiple dbus interfaces and object signals.
 *
 *  Any number of classes representing a dbus interface may be composed into
 *  a single dbus object.  The interfaces will be created first and hooked
 *  into the object space via the 'add_object_vtable' calls.  Afterwards,
 *  a signal will be emitted for the whole object to indicate all new
 *  interfaces via 'sd_bus_emit_object_added'.
 *
 *  Similar, on destruction, the interfaces are removed (unref'd) and the
 *  'sd_bus_emit_object_removed' signals are emitted.
 *
 */
template <class... Args>
struct object :
    details::compose<Args...>,
    private sdbusplus::bus::details::bus_friend
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *         - Move operations.
     *     Allowed:
     *         - Destructor.
     */
    object() = delete;
    object(const object&) = delete;
    object& operator=(const object&) = delete;
    object(object&&) = delete;
    object& operator=(object&&) = delete;

    enum class action
    {
        /** sd_bus_emit_object_{added, removed} */
        emit_object_added,
        /** sd_bus_emit_interfaces_{added, removed} */
        emit_interface_added,
        /** no automatic added signal, but sd_bus_emit_object_removed on
         *  destruct */
        defer_emit,
        /** no interface signals */
        emit_no_signals,
    };

    /** Construct an 'object' on a bus with a path.
     *
     *  @param[in] bus - The bus to place the object on.
     *  @param[in] path - The path the object resides at.
     *  @param[in] act - Set to the desired InterfacesAdded signal behavior.
     */
    object(bus_t& bus, const char* path,
           action act = action::emit_object_added) :
        details::compose<Args...>(bus, path),
        __sdbusplus_server_object_bus(get_busp(bus), bus.getInterface()),
        __sdbusplus_server_object_path(path),
        __sdbusplus_server_object_intf(bus.getInterface())
    {
        // Default ctor
        check_action(act);
    }

    ~object()
    {
        if (__sdbusplus_server_object_signalstate != action::emit_no_signals)
        {
            __sdbusplus_server_object_intf->sd_bus_emit_object_removed(
                get_busp(__sdbusplus_server_object_bus),
                __sdbusplus_server_object_path.c_str());
        }
    }

    /** Emit the 'object-added' signal, if not already sent. */
    void emit_object_added()
    {
        if (__sdbusplus_server_object_signalstate == action::defer_emit)
        {
            __sdbusplus_server_object_intf->sd_bus_emit_object_added(
                get_busp(__sdbusplus_server_object_bus),
                __sdbusplus_server_object_path.c_str());
            __sdbusplus_server_object_signalstate = action::emit_object_added;
        }
    }

  private:
    // These member names are purposefully chosen as long and, hopefully,
    // unique.  Since an object is 'composed' via multiple-inheritance,
    // all members need to have unique names to ensure there is no
    // ambiguity.
    bus_t __sdbusplus_server_object_bus;
    std::string __sdbusplus_server_object_path;
    action __sdbusplus_server_object_signalstate = action::defer_emit;
    SdBusInterface* __sdbusplus_server_object_intf;

    /** Check and run the action */
    void check_action(action act)
    {
        switch (act)
        {
            case action::emit_object_added:
                // We are wanting to call emit_object_added to set up in
                // deferred state temporarily and then emit the signal.
                __sdbusplus_server_object_signalstate = action::defer_emit;
                emit_object_added();
                break;

            case action::emit_interface_added:
                details::compose<Args...>::maybe_emit_iface_added();
                // If we are emitting at an interface level, we should never
                // also emit at the object level.
                __sdbusplus_server_object_signalstate = action::emit_no_signals;
                break;

            case action::defer_emit:
                __sdbusplus_server_object_signalstate = action::defer_emit;
                break;

            case action::emit_no_signals:
                __sdbusplus_server_object_signalstate = action::emit_no_signals;
                break;
        }
    }
};

} // namespace object

template <class... Args>
using object_t = object::object<Args...>;

} // namespace server
} // namespace sdbusplus
