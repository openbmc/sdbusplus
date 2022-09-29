#pragma once

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/sdbus.hpp>

#include <algorithm>
#include <climits>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#endif

namespace sdbusplus
{

namespace bus
{

using busp_t = sd_bus*;
struct bus;

/** @brief Get an instance of the 'default' bus. */
bus new_default();
/** @brief Get an instance of the 'user' session bus. */
bus new_user();
/** @brief Get an instance of the 'system' bus. */
bus new_system();

namespace details
{

/** @brief unique_ptr functor to release a bus reference. */
struct BusDeleter
{
    BusDeleter() = delete;
    explicit BusDeleter(SdBusInterface* interface) : m_interface(interface) {}

    void operator()(sd_bus* ptr) const
    {
        (m_interface->*deleter)(ptr);
    }

    SdBusInterface* m_interface;
    decltype(&SdBusInterface::sd_bus_unref) deleter =
        &SdBusInterface::sd_bus_unref;
};

/** @brief Convert a vector of strings to c-style char** array. */
class Strv
{
  public:
    ~Strv() = default;
    Strv() = delete;
    Strv(const Strv&) = delete;
    Strv& operator=(const Strv&) = delete;
    Strv(Strv&&) = default;
    Strv& operator=(Strv&&) = default;

    explicit Strv(const std::vector<std::string>& v)
    {
        std::transform(v.begin(), v.end(), std::back_inserter(ptrs),
                       [](const auto& i) { return i.c_str(); });
        ptrs.push_back(nullptr);
    }

    explicit operator char**()
    {
        return const_cast<char**>(&ptrs.front());
    }

  private:
    std::vector<const char*> ptrs;
};

/* @brief Alias 'bus' to a unique_ptr type for auto-release. */
using bus = std::unique_ptr<sd_bus, BusDeleter>;

struct bus_friend;

} // namespace details

/** @class bus
 *  @brief Provides C++ bindings to the sd_bus_* class functions.
 */
struct bus
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    bus() = delete;
    bus(const bus&) = delete;
    bus& operator=(const bus&) = delete;

    bus(bus&&) = default;
    bus& operator=(bus&&) = default;
    ~bus() = default;

    bus(busp_t b, sdbusplus::SdBusInterface* intf);

    /** @brief Conversion constructor from 'busp_t'.
     *
     *  Increments ref-count of the bus-pointer and releases it when done.
     */
    explicit bus(busp_t b);

    /** @brief Constructor for 'bus'.
     *
     *  Takes ownership of the bus-pointer and releases it when done.
     */
    bus(busp_t b, std::false_type);

    /** @brief Sets the bus to be closed when this handle is destroyed. */
    void set_should_close(bool shouldClose)
    {
        if (shouldClose)
        {
            _bus.get_deleter().deleter =
                &SdBusInterface::sd_bus_flush_close_unref;
        }
        else
        {
            _bus.get_deleter().deleter = &SdBusInterface::sd_bus_unref;
        }
    }

    /** @brief Release ownership of the stored bus-pointer. */
    busp_t release()
    {
        return _bus.release();
    }

    /** @brief Flush all of the outstanding work on the bus. */
    void flush()
    {
        int r = _intf->sd_bus_flush(_bus.get());
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_flush");
        }
    }

    /** @brief Close the connection to the dbus daemon. */
    void close()
    {
        _intf->sd_bus_close(_bus.get());
    }

    /** @brief Wait for new dbus messages or signals.
     *
     *  @param[in] timeout_us - Timeout in usec.
     */
    void wait(uint64_t timeout_us)
    {
        _intf->sd_bus_wait(_bus.get(), timeout_us);
    }
    void wait(std::optional<SdBusDuration> timeout = std::nullopt)
    {
        wait(timeout ? timeout->count() : UINT64_MAX);
    }

    /** @brief Process waiting dbus messages or signals. */
    auto process()
    {
        sd_bus_message* m = nullptr;
        int r = _intf->sd_bus_process(_bus.get(), &m);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_process");
        }

        return message_t(m, _intf, std::false_type());
    }

    /** @brief Process waiting dbus messages or signals, discarding unhandled.
     */
    auto process_discard()
    {
        int r = _intf->sd_bus_process(_bus.get(), nullptr);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_process discard");
        }
        return r > 0;
    }

    /** @brief Process waiting dbus messages or signals forever, discarding
     * unhandled.
     */
    [[noreturn]] void process_loop()
    {
        while (true)
        {
            process_discard();
            wait();
        }
    }

    /** @brief Claim a service name on the dbus.
     *
     *  @param[in] service - The service name to claim.
     */
    void request_name(const char* service)
    {
        int r = _intf->sd_bus_request_name(_bus.get(), service, 0);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_request_name");
        }
    }

    /** @brief Create a method_call message.
     *
     *  @param[in] service - The service to call.
     *  @param[in] objpath - The object's path for the call.
     *  @param[in] interf - The object's interface to call.
     *  @param[in] method - The object's method to call.
     *
     *  @return A newly constructed message.
     */
    auto new_method_call(const char* service, const char* objpath,
                         const char* interf, const char* method)
    {
        sd_bus_message* m = nullptr;
        int r = _intf->sd_bus_message_new_method_call(_bus.get(), &m, service,
                                                      objpath, interf, method);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_new_method_call");
        }

        return message_t(m, _intf, std::false_type());
    }

    /** @brief Create a signal message.
     *
     *  @param[in] objpath - The object's path for the signal.
     *  @param[in] interf - The object's interface for the signal.
     *  @param[in] member - The signal name.
     *
     *  @return A newly constructed message.
     */
    auto new_signal(const char* objpath, const char* interf, const char* member)
    {
        sd_bus_message* m = nullptr;
        int r = _intf->sd_bus_message_new_signal(_bus.get(), &m, objpath,
                                                 interf, member);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_new_signal");
        }

        return message_t(m, _intf, std::false_type());
    }

    /** @brief Perform a message call.
     *         Errors generated by this call come from underlying dbus
     *         related errors *AND* from any method call that results
     *         in a METHOD_ERROR. This means you do not need to check
     *         is_method_error() on the returned message.
     *
     *  @param[in] m - The method_call message.
     *  @param[in] timeout_us - The timeout for the method call.
     *
     *  @return The response message.
     */
    auto call(message_t& m, uint64_t timeout_us)
    {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        sd_bus_message* reply = nullptr;
        int r =
            _intf->sd_bus_call(_bus.get(), m.get(), timeout_us, &error, &reply);
        if (r < 0)
        {
            throw exception::SdBusError(&error, "sd_bus_call");
        }

        return message_t(reply, _intf, std::false_type());
    }
    auto call(message_t& m, std::optional<SdBusDuration> timeout = std::nullopt)
    {
        return call(m, timeout ? timeout->count() : 0);
    }

    /** @brief Perform a message call, ignoring the reply.
     *
     *  @param[in] m - The method_call message.
     *  @param[in] timeout_us - The timeout for the method call.
     */
    void call_noreply(message_t& m, uint64_t timeout_us)
    {
        sd_bus_error error = SD_BUS_ERROR_NULL;
        int r = _intf->sd_bus_call(_bus.get(), m.get(), timeout_us, &error,
                                   nullptr);
        if (r < 0)
        {
            throw exception::SdBusError(&error, "sd_bus_call noreply");
        }
    }
    auto call_noreply(message_t& m,
                      std::optional<SdBusDuration> timeout = std::nullopt)
    {
        return call_noreply(m, timeout ? timeout->count() : 0);
    }

    /** @brief Perform a message call, ignoring the reply and any errors
     *         in the dbus stack.
     *
     *  @param[in] m - The method_call message.
     *  @param[in] timeout_us - The timeout for the method call.
     */
    void call_noreply_noerror(message_t& m, uint64_t timeout_us)
    {
        try
        {
            call_noreply(m, timeout_us);
        }
        catch (const exception::SdBusError&)
        {
            // Intentionally ignoring these sd_bus errors
        }
    }
    auto call_noreply_noerror(
        message_t& m, std::optional<SdBusDuration> timeout = std::nullopt)
    {
        return call_noreply_noerror(m, timeout ? timeout->count() : 0);
    }

    /** @brief Get the bus unique name. Ex: ":1.11".
     *
     * @return The bus unique name.
     */
    auto get_unique_name()
    {
        const char* unique = nullptr;
        _intf->sd_bus_get_unique_name(_bus.get(), &unique);
        return std::string(unique);
    }

    auto get_fd()
    {
        return _intf->sd_bus_get_fd(_bus.get());
    }

    /** @brief Attach the bus with a sd-event event loop object.
     *
     *  @param[in] event - sd_event object.
     *  @param[in] priority - priority of bus event source.
     */
    void attach_event(sd_event* event, int priority)
    {
        _intf->sd_bus_attach_event(_bus.get(), event, priority);
    }

    /** @brief Detach the bus from its sd-event event loop object */
    void detach_event()
    {
        _intf->sd_bus_detach_event(_bus.get());
    }

    /** @brief Get the sd-event event loop object of the bus */
    auto get_event()
    {
        return _intf->sd_bus_get_event(_bus.get());
    }

    /** @brief Wrapper for sd_bus_emit_interfaces_added_strv
     *
     *  In general the similarly named server::object::object API should
     *  be used to manage emission of ObjectManager signals in favor
     *  of this one.  Provided here for complex usage scenarios.
     *
     *  @param[in] path - The path to forward.
     *  @param[in] ifaces - The interfaces to forward.
     */
    void emit_interfaces_added(const char* path,
                               const std::vector<std::string>& ifaces);

    /** @brief Wrapper for sd_bus_emit_interfaces_removed_strv
     *
     *  In general the similarly named server::object::object API should
     *  be used to manage emission of ObjectManager signals in favor
     *  of this one.  Provided here for complex usage scenarios.
     *
     *  @param[in] path - The path to forward.
     *  @param[in] ifaces - The interfaces to forward.
     */
    void emit_interfaces_removed(const char* path,
                                 const std::vector<std::string>& ifaces);

    /** @brief Wrapper for sd_bus_emit_object_added
     *
     *  In general the similarly named server::object::object API should
     *  be used to manage emission of ObjectManager signals in favor
     *  of this one.  Provided here for complex usage scenarios.
     *
     *  @param[in] path - The path to forward to sd_bus_emit_object_added
     */
    void emit_object_added(const char* path)
    {
        _intf->sd_bus_emit_object_added(_bus.get(), path);
    }

    /** @brief Wrapper for sd_bus_emit_object_removed
     *
     *  In general the similarly named server::object::object API should
     *  be used to manage emission of ObjectManager signals in favor
     *  of this one.  Provided here for complex usage scenarios.
     *
     *  @param[in] path - The path to forward to sd_bus_emit_object_removed
     */
    void emit_object_removed(const char* path)
    {
        _intf->sd_bus_emit_object_removed(_bus.get(), path);
    }

    /** @brief Wrapper for sd_bus_list_names.
     *
     *  @return A vector of strings containing the 'acquired' names from
     *          sd_bus_list_names.
     */
    auto list_names_acquired()
    {
        char** names = nullptr;

        _intf->sd_bus_list_names(_bus.get(), &names, nullptr);

        std::vector<std::string> result;
        for (auto ptr = names; ptr && *ptr; ++ptr)
        {
            result.push_back(*ptr);
            free(*ptr);
        }
        free(names);

        return result;
    }

    /** @brief Get the SdBusInterface used by this bus.
     *
     *  @return A pointer to the SdBusInterface used by this bus.
     */
    sdbusplus::SdBusInterface* getInterface()
    {
        return _intf;
    }

    friend struct details::bus_friend;

  protected:
    busp_t get() noexcept
    {
        return _bus.get();
    }
    sdbusplus::SdBusInterface* _intf;
    details::bus _bus;
};

inline bus::bus(busp_t b, sdbusplus::SdBusInterface* intf) :
    _intf(intf), _bus(_intf->sd_bus_ref(b), details::BusDeleter(intf))
{
    // Emitting object added causes a message to get the properties
    // which can trigger a 'transaction' in the server bindings.  If
    // the bus isn't up far enough, this causes an assert deep in
    // sd-bus code.  Get the 'unique_name' to ensure the bus is up far
    // enough to avoid the assert.
    if (b != nullptr)
    {
        get_unique_name();
    }
}

inline bus::bus(busp_t b) :
    _intf(&sdbus_impl),
    _bus(_intf->sd_bus_ref(b), details::BusDeleter(&sdbus_impl))
{
    // Emitting object added causes a message to get the properties
    // which can trigger a 'transaction' in the server bindings.  If
    // the bus isn't up far enough, this causes an assert deep in
    // sd-bus code.  Get the 'unique_name' to ensure the bus is up far
    // enough to avoid the assert.
    if (b != nullptr)
    {
        get_unique_name();
    }
}

inline bus::bus(busp_t b, std::false_type) :
    _intf(&sdbus_impl), _bus(b, details::BusDeleter(&sdbus_impl))
{
    // Emitting object added causes a message to get the properties
    // which can trigger a 'transaction' in the server bindings.  If
    // the bus isn't up far enough, this causes an assert deep in
    // sd-bus code.  Get the 'unique_name' to ensure the bus is up far
    // enough to avoid the assert.
    if (b != nullptr)
    {
        get_unique_name();
    }
}

/* Create a new default connection: system bus if root, session bus if user */
inline bus new_default()
{
    sd_bus* b = nullptr;
    sd_bus_default(&b);
    return bus(b, std::false_type());
}

/* Create a new default connection to the session bus */
inline bus new_default_user()
{
    sd_bus* b = nullptr;
    sd_bus_default_user(&b);
    return bus(b, std::false_type());
}

/* Create a new default connection to the system bus */
inline bus new_default_system()
{
    sd_bus* b = nullptr;
    sd_bus_default_system(&b);
    return bus(b, std::false_type());
}

/* WARNING: THESE ARE NOT THE FUNCTIONS YOU ARE LOOKING FOR! */
inline bus new_bus()
{
    sd_bus* b = nullptr;
    sd_bus_open(&b);
    bus bus(b, std::false_type());
    bus.set_should_close(true);
    return bus;
}

inline bus new_user()
{
    sd_bus* b = nullptr;
    sd_bus_open_user(&b);
    bus bus(b, std::false_type());
    bus.set_should_close(true);
    return bus;
}

inline bus new_system()
{
    sd_bus* b = nullptr;
    sd_bus_open_system(&b);
    bus bus(b, std::false_type());
    bus.set_should_close(true);
    return bus;
}

namespace details
{

// Some sdbusplus classes need to be able to pass the underlying bus pointer
// along to sd_bus calls, but we don't want to make it available for everyone.
// Define a class which can be inherited explicitly (intended for internal users
// only) to get the underlying bus pointer.
struct bus_friend
{
    static busp_t get_busp(sdbusplus::bus::bus& b) noexcept
    {
        return b.get();
    }
};

} // namespace details

} // namespace bus

using bus_t = bus::bus;

/** @brief Get the dbus bus from the message.
 *
 *  @return The dbus bus.
 */
inline auto message_t::get_bus() const
{
    sd_bus* b = nullptr;
    b = _intf->sd_bus_message_get_bus(_msg.get());
    return bus_t(b, _intf);
}

} // namespace sdbusplus

#ifdef __clang__
#pragma clang diagnostic pop
#endif
