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
#include <sdbusplus/utility/read_into_tuple.hpp>
#include <sdbusplus/utility/type_traits.hpp>
#include <sdbusplus/asio/detail/async_send_handler.hpp>
#include <chrono>
#include <string>
#include <experimental/tuple>
#include <boost/asio.hpp>
#include <boost/callable_traits.hpp>

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
        read_wait();
    }
    connection(boost::asio::io_service& io, sd_bus* bus) :
        sdbusplus::bus::bus(bus), io_(io), socket(io_)
    {
        socket.assign(get_fd());
        read_wait();
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
        detail::async_send_handler<typename boost::asio::handler_type<
            MessageHandler,
            void(boost::system::error_code, message::message)>::type>(
            std::move(init.completion_handler))(get(), m);
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
        return async_send(m, [handler](boost::system::error_code ec,
                                       message::message& r) {
            using FunctionTuple =
                boost::callable_traits::args_t<MessageHandler>;
            using UnpackType = typename utility::strip_first_arg<
                typename utility::decay_tuple<FunctionTuple>::type>::type;
            UnpackType responseData;
            if (!ec)
            {
                if (!utility::read_into_tuple(responseData, r))
                {
                    // Set error code if not already set
                    ec = boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument);
                }
            }
            // Note.  Callback is called whether or not the unpack was
            // sucessful to allow the user to implement their own handling
            auto response = std::tuple_cat(std::make_tuple(ec), responseData);
            std::experimental::apply(handler, response);
        });
    }

  private:
    boost::asio::io_service& io_;
    boost::asio::posix::stream_descriptor socket;

    void read_wait()
    {
        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code& ec, std::size_t) {
                if (process_discard() > 0)
                {
                    read_immediate();
                }
                else
                {
                    read_wait();
                }
            });
    }

    void read_immediate()
    {
        io_.post([&] {
            if (process_discard() > 0)
            {
                read_immediate();
            }
            else
            {
                read_wait();
            }
        });
    }
};

} // namespace asio

} // namespace sdbusplus
