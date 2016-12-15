#pragma once

#include <memory>
#include <sstream>

#include <sdbusplus/slot.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

namespace sdbusplus
{

namespace bus
{

namespace watch
{

struct watch
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    watch() = delete;
    watch(const watch&) = delete;
    watch& operator=(const watch&) = delete;
    watch(watch&&) = default;
    watch& operator=(watch&&) = default;
    ~watch() = default;

    /** @brief Register a service watcher.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] service_name - The name of the service..
     *  @param[in] on_connected - The callback when service connected to the bus.
     *  @param[in] on_disconnected - The callback when service disconnected from the bus.
     *  @param[in] on_owner_changed - The callback when owner of the service changed.
     */
    watch(sdbusplus::bus::bus& bus,
        const char* service_name,
        std::function<void(void)> on_connected = nullptr,
        std::function<void(void)> on_disconnected = nullptr,
        std::function<void(void)> on_owner_changed = nullptr) :
            on_connected_{on_connected},
            on_disconnected_{on_disconnected},
            on_owner_changed_{on_owner_changed}
    {
        std::ostringstream buffer;
        buffer << "sender='org.freedesktop.DBus',"
            "type='signal',"
            "interface='org.freedesktop.DBus',"
            "member='NameOwnerChanged',"
            "path='/org/freedesktop/DBus',"
            "arg0='" << service_name << "'";
        auto match = buffer.str();

        match_ = std::make_unique<sdbusplus::bus::match_t>(bus, match.c_str(), on_match, this);
    }

    private:
        static int on_match(sd_bus_message *m, void *userdata, sd_bus_error *ret_error)
        {
            auto o = static_cast<watch*>(userdata);

            auto msg = sdbusplus::message::message(m);

            std::string name, old_owner, new_owner;
            msg.read(name, old_owner, new_owner);

            if (!new_owner.empty() && old_owner.empty())
            {
                if (o->on_connected_) o->on_connected_();
            }
            else if (new_owner.empty() && !old_owner.empty())
            {
                if (o->on_disconnected_) o->on_disconnected_();
            }
            else
            {
                if (o->on_owner_changed_) o->on_owner_changed_();
            }

            return 0;
        }

    private:
        std::unique_ptr<sdbusplus::bus::match_t> match_;
        std::function<void(void)> on_connected_;
        std::function<void(void)> on_disconnected_;
        std::function<void(void)> on_owner_changed_;
};

} // namespace watch

using watch_t = watch::watch;

} // namespace bus
} // namespace sdbusplus
