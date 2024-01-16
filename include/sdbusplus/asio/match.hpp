#pragma once

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/slot.hpp>

#include <functional>
#include <memory>
#include <string>

namespace sdbusplus::asio
{

class match
{
  public:
    // It captures this for the callback, so it's not copyable nor movable
    match(match&) = delete;
    match& operator=(match&) = delete;
    match(match&&) = delete;
    match& operator=(match&&) = delete;

    // The same callback type as sdbusplus::match_t
    using callback_t = std::function<void(sdbusplus::message_t&)>;

    /** @brief Register a signal match.
     *
     *  @param[in] conn - The asio connection to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     */
    match(sdbusplus::asio::connection& conn, const char* _match,
          callback_t callback) :
        _callback(std::make_unique<callback_t>(std::move(callback))),
        _slot(makeMatch(conn, _match, this))
    {}

    inline match(sdbusplus::asio::connection& conn, const std::string& _match,
                 callback_t callback) :
        match(conn, _match.c_str(), std::move(callback))

    {}

    /** @brief Register a signal match with install callback
     *
     *  @param[in] conn - The asio connection to register on.
     *  @param[in] match - The match to register.
     *  @param[in] callback - The callback for matches.
     *  @param[in] install_callback - The callback for matche install
     */
    match(sdbusplus::asio::connection& conn, const char* _match,
          callback_t callback, callback_t install_callback) :
        _callback(std::make_unique<callback_t>(std::move(callback))),
        install_callback(
            std::make_unique<callback_t>(std::move(install_callback))),
        _slot(makeMatch(conn, _match, this))
    {}

    inline match(sdbusplus::asio::connection& conn, const std::string& _match,
                 callback_t callback, callback_t install_callback) :
        match(conn, _match.c_str(), std::move(callback),
              std::move(install_callback))
    {}

  private:
    std::unique_ptr<callback_t> _callback;
    std::unique_ptr<callback_t> install_callback;
    slot_t _slot;

    static slot_t makeMatch(sdbusplus::asio::connection& conn,
                            const char* _match, void* context)
    {
        sd_bus_slot* slot;
        auto intf = conn.getInterface();
        int r = intf->sd_bus_add_match_async(conn.get(), &slot, _match,
                                             on_event_static, on_install_static,
                                             context);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_match_async");
        }
        return slot_t{slot, intf};
    }

    static int on_event_static(sd_bus_message* m, void* userdata, sd_bus_error*)
    {
        auto self = reinterpret_cast<match*>(userdata);
        message_t message{m};
        (*self->_callback)(message);
        return 0;
    }

    static int on_install_static(sd_bus_message* m, void* userdata,
                                 sd_bus_error*)
    {
        auto self = reinterpret_cast<match*>(userdata);
        if (self->install_callback)
        {
            message_t message{m};
            (*self->install_callback)(message);
        }
        return 0;
    }
};

} // namespace sdbusplus::asio
