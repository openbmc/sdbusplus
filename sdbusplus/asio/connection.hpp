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

#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/type_traits.hpp>
#include <sdbusplus/utility/function_traits.hpp>
#include <sdbusplus/asio/detail/async_send_op.hpp>
#include <chrono>
#include <string>
#include <boost/asio.hpp>

namespace sdbusplus
{

namespace asio
{

/// Root D-Bus IO object
/**
 * A connection to a bus, through which messages may be sent or received.
 */
class connection : public sdbusplus::bus::bus
{
  public:
    // default to system bus
    connection(boost::asio::io_service& io) :
        sdbusplus::bus::bus(sdbusplus::bus::new_system()), io_(io), socket(io_)
    {
        socket.assign(get_fd());
        do_read();
    }
    connection(boost::asio::io_service& io, sd_bus* bus) :
        sdbusplus::bus::bus(bus), io_(io), socket(io_)
    {
        socket.assign(get_fd());
        do_read();
    }
    ~connection()
    {
        // The FD will be closed by the socket object, so assign null to the
        // sd_bus object to avoid a double close()  Ignore return codes here,
        // because there's nothing we can do about errors
        socket.release();
    }

    template <typename MessageHandler>
    inline BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
                                         void(boost::system::error_code,
                                              message::message))
        async_send(message::message& m, MessageHandler&& handler)
    {
        boost::asio::async_completion<
            MessageHandler, void(boost::system::error_code, message::message)>
            init(handler);
        detail::async_send_op<typename boost::asio::handler_type<
            MessageHandler,
            void(boost::system::error_code, message::message)>::type>(
            io_, std::move(init.completion_handler))(get(), m);
        do_write();
        return init.result.get();
    }

    template <typename MessageHandler, typename... InputArgs>
    auto async_method_call(MessageHandler handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        message::message m = new_method_call(service.c_str(), objpath.c_str(),
                                             interf.c_str(), method.c_str());
        m.append(a...);
        return async_send(
            m, [handler](boost::system::error_code ec, message::message& r) {
                // Make a copy of the error code so we can modify it
                using function_tuple =
                    typename function_traits<MessageHandler>::decayed_arg_types;
                using unpack_type =
                    typename utility::strip_first_arg<function_tuple>::type;
                unpack_type response_args;
                if (!ec)
                {
                    if (!unpack_into_tuple(response_args, r))
                    {
                        // Set error code
                        ec = boost::system::errc::make_error_code(
                            boost::system::errc::invalid_argument);
                    }
                }
                // Note.  Callback is called whether or not the unpack was
                // sucessful to allow the user to implement their own handling
                index_apply<std::tuple_size<unpack_type>{}>([&](auto... Is) {
                    handler(ec, std::get<Is>(response_args)...);
                });
            });
    }

    void request_name(const char* service)
    {
        do_write();
        bus::request_name(service);
    }

    sdbusplus::message::message call(message::message& m,
                                     uint64_t timeout_us = 0)
    {
        do_write();
        return bus::call(m, timeout_us);
    }

    void call_noreply(message::message& m, uint64_t timeout_us = 0)
    {
        do_write();
        bus::call_noreply(m, timeout_us);
    }

  private:
    boost::asio::io_service& io_;
    boost::asio::posix::stream_descriptor socket;
    bool doingWrite = false;

    void do_read(void)
    {
        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code& ec, std::size_t) {
                process_discard();
                do_read();
            });
    }

    void do_write(void)
    {
        if (doingWrite)
        {
            return;
        }
        doingWrite = true;

        socket.async_write_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code& ec, std::size_t) {
                doingWrite = false;
                process_discard();
                if (sd_bus_get_events(get()) | POLLOUT)
                {
                    do_write();
                }
            });
    }
};

} // namespace asio

} // namespace sdbusplus
