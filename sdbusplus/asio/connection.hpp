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

#include <boost/asio.hpp>
#include <boost/callable_traits.hpp>
#include <chrono>
#include <experimental/tuple>
#include <sdbusplus/asio/detail/async_send_handler.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/read_into_tuple.hpp>
#include <sdbusplus/utility/type_traits.hpp>
#include <string>

namespace sdbusplus
{

namespace asio
{

namespace details
{

/* A simple class to integrate the sd_event_loop into the boost::asio io_service
 * in case a boost::asio user needs sd_events
 */
class sd_event_wrapper
{
  public:
    sd_event_wrapper(boost::asio::io_service& io) :
        evt(nullptr), descriptor(io), io(io)
    {
        sd_event_default(&evt);
        if (evt)
        {
            descriptor.assign(sd_event_get_fd(evt));
            async_run();
        }
    }
    sd_event_wrapper(sd_event* evt, boost::asio::io_service& io) :
        evt(evt), descriptor(io), io(io)
    {
        sd_event_ref(evt);
        if (evt)
        {
            descriptor.assign(sd_event_get_fd(evt));
            async_run();
        }
    }
    ~sd_event_wrapper()
    {
        sd_event_unref(evt);
    }
    // process one event step in the queue
    // return true if the queue is still alive
    void run()
    {
        int ret;
        int state = sd_event_get_state(evt);
        switch (state)
        {
            case SD_EVENT_INITIAL:
                ret = sd_event_prepare(evt);
                if (ret > 0)
                {
                    async_run();
                }
                else if (ret == 0)
                {
                    async_wait();
                }
                break;
            case SD_EVENT_ARMED:
                ret = sd_event_wait(evt, 0);
                if (ret >= 0)
                {
                    async_run();
                }
                break;
            case SD_EVENT_PENDING:
                ret = sd_event_dispatch(evt);
                if (ret > 0)
                {
                    async_run();
                }
                break;
            case SD_EVENT_FINISHED:
                break;
            default:
                // throw something?
                // doing nothing will break out of the async loop
                break;
        }
    }
    sd_event* get() const
    {
        return evt;
    }

  private:
    void async_run()
    {
        io.post([this]() { run(); });
    }
    void async_wait()
    {
        descriptor.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                              [this](const boost::system::error_code& error) {
                                  if (!error)
                                  {
                                      run();
                                  }
                              });
    }
    sd_event* evt;
    boost::asio::posix::stream_descriptor descriptor;
    boost::asio::io_service& io;
};
} // namespace details

/// Root D-Bus IO object
/**
 * A connection to a bus, through which messages may be sent or received.
 */
class connection : public sdbusplus::bus::bus
{
  public:
    // default to system bus
    connection(boost::asio::io_service& io) :
        sdbusplus::bus::bus(sdbusplus::bus::new_system()), io_(io), socket(io_),
        sd_events(nullptr)
    {
        socket.assign(get_fd());
        read_wait();
    }
    connection(boost::asio::io_service& io, sd_bus* bus) :
        sdbusplus::bus::bus(bus), io_(io), socket(io_), sd_events(nullptr)
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

    void enable_sd_events(sd_event* evt = nullptr)
    {
        if (!evt)
        {
            sd_event_default(&evt);
        }
        sd_events = std::make_unique<details::sd_event_wrapper>(evt, io_);
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
            // successful to allow the user to implement their own handling
            auto response = std::tuple_cat(std::make_tuple(ec), responseData);
            std::experimental::apply(handler, response);
        });
    }

  private:
    boost::asio::io_service& io_;
    boost::asio::posix::stream_descriptor socket;
    std::unique_ptr<details::sd_event_wrapper> sd_events;

    void read_wait()
    {
        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code& ec, std::size_t) {
                if (process_discard())
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
            if (process_discard())
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
