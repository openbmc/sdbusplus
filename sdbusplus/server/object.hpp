#pragma once
#include <sdbusplus/bus.hpp>

namespace sdbusplus
{

namespace server
{

namespace object
{

namespace details
{

/** Templates to allow multiple inheritence via template parameters.
 *
 *  These allow an object to group multiple dbus interface bindings into a
 *  single class.
 */
template <class T, class... Rest> struct compose_impl : T, compose_impl<Rest...>
{
    compose_impl(bus::bus &bus, const char *path) :
        T(bus, path), compose_impl<Rest...>(bus, path)
    {
    }
};

/** Specialization for single element. */
template <class T> struct compose_impl<T> : T
{
    compose_impl(bus::bus &bus, const char *path) : T(bus, path)
    {
    }
};

/** Default compose operation for variadic arguments. */
template <class... Args> struct compose : compose_impl<Args...>
{
    compose(bus::bus &bus, const char *path) : compose_impl<Args...>(bus, path)
    {
    }
};

/** Specialization for zero variadic arguments. */
template <> struct compose<>
{
    compose(bus::bus &bus, const char *path)
    {
    }
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
template <class... Args> struct object : details::compose<Args...>
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    object() = delete;
    object(const object &) = delete;
    object &operator=(const object &) = delete;
    object(object &&) = default;
    object &operator=(object &&) = default;

    /** Construct an 'object' on a bus with a path.
     *
     *  @param[in] bus - The bus to place the object on.
     *  @param[in] path - The path the object resides at.
     *  @param[in] deferSignal - Set to true if emit_object_added should be
     *                           deferred.  This would likely be true if the
     *                           object needs custom property init before the
     *                           signal can be sent.
     */
    object(bus::bus &bus, const char *path, bool deferSignal = false) :
        details::compose<Args...>(bus, path),
        __sdbusplus_server_object_bus(bus.get()),
        __sdbusplus_server_object_path(path),
        __sdbusplus_server_object_emitremoved(false)
    {
        if (!deferSignal)
        {
            emit_object_added();
        }
    }

    ~object()
    {
        if (__sdbusplus_server_object_emitremoved)
        {
            sd_bus_emit_object_removed(__sdbusplus_server_object_bus.get(),
                                       __sdbusplus_server_object_path.c_str());
        }
    }

    /** Emit the 'object-added' signal, if not already sent. */
    void emit_object_added()
    {
        if (!__sdbusplus_server_object_emitremoved)
        {
            sd_bus_emit_object_added(__sdbusplus_server_object_bus.get(),
                                     __sdbusplus_server_object_path.c_str());
            __sdbusplus_server_object_emitremoved = true;
        }
    }

  private:
    // These member names are purposefully chosen as long and, hopefully,
    // unique.  Since an object is 'composed' via multiple-inheritence,
    // all members need to have unique names to ensure there is no
    // ambiguity.
    bus::bus __sdbusplus_server_object_bus;
    std::string __sdbusplus_server_object_path;
    bool __sdbusplus_server_object_emitremoved;
};

} // namespace object

template <class... Args> using object_t = object::object<Args...>;

} // namespace server
} // namespace sdbusplus
