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

/* Class meant for converting a static callback, and void* userdata from sd-bus
 * back into a structured class to be returned to the user.
 */
template <typename CompletionToken>
struct unpack_userdata
{
    CompletionToken handler_;

    static int do_unpack(sd_bus_message* mesg, void* userdata,
                         sd_bus_error* /*error*/)
    {
        if (userdata == nullptr || mesg == nullptr)
        {
            return -1;
        }
        using self_t = unpack_userdata<CompletionToken>;
        self_t* context = static_cast<self_t*>(userdata);
        message_t message(mesg);
        auto ec = make_error_code(
            static_cast<boost::system::errc::errc_t>(message.get_errno()));
        context->handler_(ec, std::move(message));
        return 1;
    }
};

struct async_send_handler
{
    sd_bus* bus;
    message_t& mesg;
    uint64_t timeout;
    async_send_handler(sd_bus* busIn, message_t& mesgIn, uint64_t timeoutIn) :
        bus(busIn), mesg(mesgIn), timeout(timeoutIn){};

    template <typename CompletionToken>
    void operator()(CompletionToken&& token)
    {
        using unpack_t = unpack_userdata<CompletionToken>;
        unpack_t* context = new unpack_t(std::move(token));
        int ec = sd_bus_call_async(bus, NULL, mesg.get(), &unpack_t::do_unpack,
                                   context, timeout);
        if (ec < 0)
        {
            // add a deleter to context because handler may throw
            std::unique_ptr<unpack_t> safe_context(context);
            auto err =
                make_error_code(static_cast<boost::system::errc::errc_t>(ec));
            context->handler_(err, mesg);
        }
    }
};
} // namespace detail
} // namespace asio
} // namespace sdbusplus
