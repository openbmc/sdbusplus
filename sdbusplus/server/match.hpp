#pragma once

#include <sdbusplus/slot.hpp>
#include <sdbusplus/bus.hpp>

namespace sdbusplus
{

namespace server
{

namespace match
{

namespace details
{

struct match_impl
{
    match_impl() = delete;
    match_impl(const match_impl&) = delete;
    match_impl& operator=(const match_impl&) = delete;
    match_impl(match_impl&&) = delete;
    match_impl& operator=(match_impl&&) = delete;
    ~match_impl() = default;

    match_impl(sdbusplus::bus::bus& bus, const char* match,
          sd_bus_message_handler_t handler, void* context = nullptr)
                : _slot(nullptr), _callback(nullptr)
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_match(bus.get(), &slot, match, handler, context);

        _slot = decltype(_slot){slot};
    }

    template <class Callable>
    explicit match_impl(sdbusplus::bus::bus& bus,
                   const char* match,
                   Callable&& cb)
                : _slot(nullptr),
                _callback(std::make_unique<callback<Callable>>(
                            std::forward<Callable>(cb)))
    {
        sd_bus_slot* slot = nullptr;
        sd_bus_add_match(
                bus.get(), &slot, match, match_impl::sd_callback, this);

        _slot = decltype(_slot){slot};
    }

    private:
        struct callback_base
        {
            virtual ~callback_base() = default;
            virtual void operator()(message::message& m) = 0;
        };

        template <class Callable>
        struct callback final : public callback_base
        {
            explicit callback(Callable&& cb)
                : _cb(std::forward<Callable>(cb)) {}

            void operator()(message::message& m)
            {
                _cb(m);
            }

            Callable _cb;
        };

        static auto sd_callback(
                sd_bus_message* m, void* context, sd_bus_error* e)
        {
            auto me = static_cast<match_impl *>(context);
            auto& callback = *me->_callback;
            message::message msg(m);
            callback(msg);
            return 0;
        }

        slot::slot _slot;
        std::unique_ptr<callback_base> _callback;
};

} // namespace details

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
                : _impl(std::make_unique<details::match_impl>(
                            bus, match, handler, context))
    {
    }

    /** @brief Register a signal match.
     *
     *  @param[in] bus - The bus to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    template <class Callable>
    explicit match(
            sdbusplus::bus::bus& bus,
            const char* match,
            Callable&& cb) :
        _impl(std::make_unique<details::match_impl>(
                    bus, match, std::forward<Callable>(cb)))
    {
    }

    private:
        std::unique_ptr<details::match_impl> _impl;
};

} // namespace match

using match_t = match::match;

} // namespace server
} // namespace sdbusplus
