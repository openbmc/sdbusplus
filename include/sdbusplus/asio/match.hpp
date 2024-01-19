#pragma once

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus::asio
{

enum class WatchEventType
{
    InstallSucceeded,
    InstallError,
    Message,
    MessageError,
};

class match
{
  private:
    static int on_message_event_static(sd_bus_message* m, void* userdata,
                                       sd_bus_error* ret_error)
    {
        return reinterpret_cast<match*>(userdata)->on_message_event(
            m, ret_error);
    }

    static int on_install_static(sd_bus_message* m, void* userdata,
                                 sd_bus_error* ret_error)
    {
        return reinterpret_cast<match*>(userdata)->on_install(m, ret_error);
    }

    int on_message_event(sd_bus_message* m, sd_bus_error* ret_error)
    {
        WatchEventType ret = WatchEventType::Message;
        if (ret_error != nullptr && intf->sd_bus_error_is_set(ret_error))
        {
            ret = WatchEventType::MessageError;
        }
        sdbusplus::message_t msg(m, intf);
        callback(ret, msg);
        return 0;
    }

    int on_install(sd_bus_message* m, sd_bus_error* ret_error)
    {
        WatchEventType ret = WatchEventType::InstallSucceeded;
        if (ret_error != nullptr && intf->sd_bus_error_is_set(ret_error))
        {
            ret = WatchEventType::InstallError;
            slot.reset();
        }
        sdbusplus::message_t msg(m, intf);
        callback(ret, msg);

        return 0;
    }

    using callback_t =
        std::function<void(WatchEventType, sdbusplus::message_t&)>;
    callback_t callback;
    std::optional<slot_t> slot;

    sdbusplus::SdBusInterface* intf;

  public:
    match(sdbusplus::asio::connection& conn, const char* match_expr,
          callback_t&& callbackIn) : callback(std::move(callbackIn))
    {
        sd_bus_slot* slot_ret;
        intf = conn.getInterface();
        int r = -1;
        if (intf != nullptr)
        {
            r = intf->sd_bus_add_match_async(conn.get(), &slot_ret, match_expr,
                                             on_message_event_static,
                                             on_install_static, this);
        }
        if (r < 0)
        {
            // TODO figure out how to construct an empty sdbusplus::message_t
            // sdbusplus::message_t msg();
            // callback(WatchEventType::InstallError, msg);
            return;
        }
        slot.emplace(slot_ret, intf);
    }
};
} // namespace sdbusplus::asio
