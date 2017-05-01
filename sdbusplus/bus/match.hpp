#pragma once

#include <functional>
#include <memory>
#include <sdbusplus/slot.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus
{

namespace bus
{

namespace match
{

struct match
{
    /* Define all of the basic class operations:
     *     Not allowed:
     *         - Default constructor to avoid nullptrs.
     *         - Copy operations due to internal unique_ptr.
     *     Allowed:
     *         - Move operations.
     *         - Destructor.
     */
    match() = delete;
    match(const match&) = delete;
    match& operator=(const match&) = delete;
    match(match&&) = default;
    match& operator=(match&&) = default;
    ~match() = default;

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] handler - The callback for matches.
     *  @param[in] context - An optional context to pass to the handler.
     */
    match(sdbusplus::bus::bus& bus, const char* match,
          sd_bus_message_handler_t handler, void* context = nullptr)
                : _slot(nullptr)
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_match(bus.get(), &slot, match, handler, context);

        _slot = decltype(_slot){slot};
    }

    using callback_t = std::function<void(sdbusplus::message::message&)>;

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    match(sdbusplus::bus::bus& bus, const char* match,
          callback_t callback)
                : _slot(nullptr),
                  _callback(std::make_unique<callback_t>(std::move(callback)))
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_match(bus.get(), &slot, match, callCallback,
                         _callback.get());

        _slot = decltype(_slot){slot};
    }

    private:
        slot::slot _slot;
        std::unique_ptr<callback_t> _callback = nullptr;

        static int callCallback(sd_bus_message *m, void* context,
                                sd_bus_error* e)
        {
            auto c = static_cast<callback_t*>(context);
            message::message message{m};

            (*c)(message);

            return 0;
        }
};

} // namespace match

using match_t = match::match;

} // namespace bus
} // namespace sdbusplus
