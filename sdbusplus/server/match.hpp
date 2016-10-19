#pragma once

#include <sdbusplus/slot.hpp>
#include <sdbusplus/bus.hpp>

namespace sdbusplus
{

namespace server
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

    private:
        slot::slot _slot;

};

} // namespace match

using match_t = match::match;

} // namespace server
} // namespace sdbusplus
