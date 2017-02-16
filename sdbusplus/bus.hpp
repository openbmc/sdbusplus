#pragma once

#include <memory>
#include <climits>
#include <vector>
#include <string>
#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>
#include <sdbusplus/message.hpp>

namespace sdbusplus
{

// Forward declare.
namespace server { namespace interface { struct interface; } }
namespace server { namespace manager { struct manager; } }
namespace server { namespace object { template<class...> struct object; } }
namespace server { namespace match { struct match; } }

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
        sd_bus_unref(ptr);
    }
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
            : strings(), ptrs()
        {
            for (const auto& s: v)
            {
                strings.push_back(std::vector<char>({s.cbegin(), s.cend()}));
                strings.back().push_back('\0');
                ptrs.push_back(&strings.back()[0]);
            }

            ptrs.push_back(nullptr);
        }

        explicit operator char**()
        {
            return &ptrs[0];
        }

    private:
        std::vector<std::vector<char>> strings;
        std::vector<char *> ptrs;
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
    void wait(uint64_t timeout_us = ULLONG_MAX)
    {
        sd_bus_wait(_bus.get(), timeout_us);
    }

    /** @brief Process waiting dbus messages or signals. */
    auto process()
    {
        sd_bus_message* m = nullptr;
        sd_bus_process(_bus.get(), &m);

        return message::message(m, std::false_type());
    }

    /** @brief Process waiting dbus messages or signals, discarding unhandled.
     */
    void process_discard()
    {
        sd_bus_process(_bus.get(), nullptr);
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

        return message::message(m, std::false_type());
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
        sd_bus_message_new_signal(_bus.get(), &m, objpath, interf, member);

        return message::message(m, std::false_type());
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

        return message::message(reply, std::false_type());
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

    /** @brief Get the bus unique name. Ex: ":1.11".
      *
      * @return The bus unique name.
      */
    auto get_unique_name()
    {
        const char* unique = nullptr;
        sd_bus_get_unique_name(_bus.get(), &unique);
        return std::string(unique);
    }

    /** @brief Attach the bus with a sd-event event loop object.
     *
     *  @param[in] event - sd_event object.
     *  @param[in] priority - priority of bus event source.
     */
    void attach_event(sd_event* event, int priority)
    {
        sd_bus_attach_event(_bus.get(), event, priority);
    }

    /** @brief Detach the bus from its sd-event event loop object */
    void detach_event()
    {
        sd_bus_detach_event(_bus.get());
    }

    /** @brief Get the sd-event event loop object of the bus */
    auto get_event()
    {
        return sd_bus_get_event(_bus.get());
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
                               const std::vector<std::string>& ifaces)
    {
        details::Strv s{ifaces};
        sd_bus_emit_interfaces_added_strv(_bus.get(),
                                          path,
                                          static_cast<char**>(s));
    }

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
                                 const std::vector<std::string>& ifaces)
    {
        details::Strv s{ifaces};
        sd_bus_emit_interfaces_removed_strv(_bus.get(),
                                            path,
                                            static_cast<char**>(s));
    }

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
        sd_bus_emit_object_added(_bus.get(), path);
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
        sd_bus_emit_object_removed(_bus.get(), path);
    }

    friend struct server::interface::interface;
    friend struct server::manager::manager;
    template<class... Args> friend struct server::object::object;
    friend struct server::match::match;

    private:
        busp_t get() { return _bus.get(); }
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

/** @brief Get the dbus bus from the message.
 *
 *  @return The dbus bus.
 */
inline auto message::message::get_bus()
{
    sd_bus* b = nullptr;
    b = sd_bus_message_get_bus(_msg.get());
    return bus::bus(sd_bus_ref(b));
}

} // namespace sdbusplus
