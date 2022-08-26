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

#include <boost/system/error_code.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

namespace sdbusplus
{
namespace asio
{
namespace detail
{
template <typename Handler>
struct async_send_handler
{
    Handler handler_;
    async_send_handler(Handler&& handler) : handler_(std::move(handler)) {}
    async_send_handler(Handler& handler) : handler_(handler) {}
    void operator()(sd_bus* conn, message_t& mesg, uint64_t timeout)
    {
        async_send_handler* context = new async_send_handler(std::move(*this));
        int ec = sd_bus_call_async(conn, NULL, mesg.get(), &callback, context,
                                   timeout);
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
                        sd_bus_error* /*error*/)
    {
        if (userdata == nullptr || mesg == nullptr)
        {
            return -1;
        }
        std::unique_ptr<async_send_handler> context(
            static_cast<async_send_handler*>(userdata));
        message_t message(mesg);
        auto ec = make_error_code(
            static_cast<boost::system::errc::errc_t>(message.get_errno()));
        context->handler_(ec, message);
        return 1;
    }
};
} // namespace detail
} // namespace asio
} // namespace sdbusplus
