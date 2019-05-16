/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include <systemd/sd-bus.h>

#include <boost/asio.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus
{
namespace asio
{
namespace detail
{

struct sdbus_error_category : public boost::system::error_category
{
    virtual ~sdbus_error_category() = default;

    sdbus_error_category() :
        boost::system::error_category(0xABCDEF) // FIXME: what is the id about?
    {
    }

    void set_name(const char* name)
    {
        sdbus_name = name;
    }
    void set_message(const std::string& message)
    {
        sdbus_message = message;
    }

    const char* name() const BOOST_NOEXCEPT override
    {
        return sdbus_name;
    }
    std::string message(int /*ev*/) const override
    {
        return sdbus_message;
    }
    const char* sdbus_name;
    std::string sdbus_message;
};

inline sdbus_error_category& sdbus_category() BOOST_NOEXCEPT
{
    // NOTE: This instance is NOT const and is shared in differnt DBus calls
    static thread_local sdbus_error_category sdbus_category_instance;
    return sdbus_category_instance;
}

template <typename Handler>
struct async_send_handler
{
    Handler handler_;
    async_send_handler(Handler&& handler) : handler_(std::move(handler))
    {
    }
    async_send_handler(Handler& handler) : handler_(handler)
    {
    }
    void operator()(sd_bus* conn, message::message& mesg)
    {
        async_send_handler* context = new async_send_handler(std::move(*this));
        // 0 is the default timeout
        int ec =
            sd_bus_call_async(conn, NULL, mesg.get(), &callback, context, 0);
        if (ec < 0)
        {
            // add a deleter to context because handler may throw
            std::unique_ptr<async_send_handler> safe_context(context);
            auto err =
                make_error_code(static_cast<boost::system::errc::errc_t>(ec));
            context->handler_(err, mesg);
        }
    }
    static int callback(sd_bus_message* mesg, void* userdata,
                        sd_bus_error* error)
    {
        if (userdata == nullptr || mesg == nullptr)
        {
            return -1;
        }
        std::unique_ptr<async_send_handler> context(
            static_cast<async_send_handler*>(userdata));
        message::message message(mesg);
        auto ec = make_error_code(
            static_cast<boost::system::errc::errc_t>(message.get_errno()));
        auto e = sd_bus_message_get_error(mesg);
        // In this callback, the error is actually in message rather than the
        // `error` parameter.
        // TODO: maybe report this issue to systemd?
        if (sd_bus_error_is_set(e))
        {
            // Override the error_category by sdbus_category
            auto& sdbus_ec = sdbusplus::asio::detail::sdbus_category();
            sdbus_ec.set_name(e->name);
            sdbus_ec.set_message(e->message);
            ec.assign(sd_bus_error_get_errno(e), sdbus_ec);
        }
        context->handler_(ec, message);
        return 1;
    }
};
} // namespace detail
} // namespace asio
} // namespace sdbusplus
