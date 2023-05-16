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

#include <functional>
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
struct unpack_userdata
{
    using callback_type = std::function<void(const boost::system::error_code&, message_t&)>;
    callback_type handler_;

    static int do_unpack(sd_bus_message* mesg, void* userdata,
                         sd_bus_error* /*error*/)
    {
        if (userdata == nullptr)
        {
            return -EINVAL;
        }

        // Take RAII ownership of the pointer again
        std::unique_ptr<unpack_userdata> context(static_cast<unpack_userdata*>(userdata));

        if (mesg == nullptr)
        {
            return -EINVAL;
        }
        message_t message(mesg);
        auto ec = make_error_code(
            static_cast<boost::system::errc::errc_t>(message.get_errno()));
        context->handler_(ec, message);
        return 0;
    }

    explicit unpack_userdata(callback_type&& handler) :
        handler_(std::move(handler))
    {}
};

class async_send_handler
{
    sd_bus* bus;
    message_t& mesg;
    uint64_t timeout;
    using callback_type = std::function<void(const boost::system::error_code&, message_t&)>;

  public:
    void operator()(callback_type&& token)
    {
        auto context = std::make_unique<unpack_userdata>(std::move(token));
        int ec = sd_bus_call_async(bus, nullptr, mesg.get(),
                                   &unpack_userdata::do_unpack, context.get(),
                                   timeout);
        if (ec < 0)
        {
            auto err =
                make_error_code(static_cast<boost::system::errc::errc_t>(ec));
            context->handler_(err, mesg);
            return;
        }
        // If the call succeeded, sd-bus owns the pointer now, so release it
        // without freeing.
        context.release();
    }

    async_send_handler(sd_bus* busIn, message_t& mesgIn, uint64_t timeoutIn) :
        bus(busIn), mesg(mesgIn), timeout(timeoutIn)
    {}
};
} // namespace detail
} // namespace asio
} // namespace sdbusplus
