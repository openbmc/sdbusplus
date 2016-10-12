#pragma once

#include <memory>
#include <systemd/sd-bus.h>
#include <sdbusplus/message.hpp>
#include <sdbusplus/vtable.hpp>

namespace sdbusplus
{
namespace bus
{

using busp_t = sd_bus*;
class bus;

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
    void operator()(sd_bus* ptr) const
    {
        sd_bus_flush_close_unref(ptr);
    }
};

/* @brief Alias 'bus' to a unique_ptr type for auto-release. */
using bus = std::unique_ptr<sd_bus, BusDeleter>;

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

    /** @brief Conversion constructor from 'busp_t'.
     *
     *  Takes ownership of the bus-pointer and releases it when done.
     */
    explicit bus(busp_t b) : _bus(b) {}

    /** @brief Release ownership of the stored bus-pointer. */
    busp_t release() { return _bus.release(); }

    /** @brief Wait for new dbus messages or signals.
     *
     *  @param[in] timeout_us - Timeout in usec.
     */
    void wait(uint64_t timeout_us = 0)
    {
        sd_bus_wait(_bus.get(), timeout_us);
    }

    /** @brief Process waiting dbus messages or signals. */
    auto process()
    {
        sd_bus_message* m = nullptr;
        sd_bus_process(_bus.get(), &m);

        return message::message(m);
    }

    /** @brief Claim a service name on the dbus.
     *
     *  @param[in] service - The service name to claim.
     */
    void request_name(const char* service)
    {
        sd_bus_request_name(_bus.get(), service, 0);
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
        sd_bus_message_new_method_call(_bus.get(), &m, service, objpath,
                                       interf, method);

        return message::message(m);
    }

    /** @brief Perform a message call.
     *
     *  @param[in] m - The method_call message.
     *  @param[in] timeout_us - The timeout for the method call.
     *
     *  @return The response message.
     */
    auto call(message::message& m, uint64_t timeout_us = 0)
    {
        sd_bus_message* reply = nullptr;
        sd_bus_call(_bus.get(), m.get(), timeout_us, nullptr, &reply);

        return reply;
    }

    /** @brief Perform a message call, ignoring the reply.
     *
     *  @param[in] m - The method_call message.
     *  @param[in] timeout_us - The timeout for the method call.
     */
    void call_noreply(message::message& m, uint64_t timeout_us = 0)
    {
        sd_bus_call(_bus.get(), m.get(), timeout_us, nullptr, nullptr);
    }

    void add_object_manager(const char *path, sd_bus_slot **slot = nullptr)
    {
        sd_bus_add_object_manager(_bus.get(), slot, path);
    }

    void add_object_vtable(const char *path, const char *iface,
		           const sdbusplus::vtable::vtable_t &vtbl,
		           sd_bus_slot **slot = nullptr,
		           void *data = nullptr)
    {
        sd_bus_add_object_vtable(_bus.get(), slot, path, iface, &vtbl, data);
    }

    private:
        details::bus _bus;
};

inline bus new_default()
{
    sd_bus* b = nullptr;
    sd_bus_open(&b);
    return bus(b);
}

inline bus new_user()
{
    sd_bus* b = nullptr;
    sd_bus_open_user(&b);
    return bus(b);
}

inline bus new_system()
{
    sd_bus* b = nullptr;
    sd_bus_open_system(&b);
    return bus(b);
}


} // namespace bus

} // namespace sdbusplus
