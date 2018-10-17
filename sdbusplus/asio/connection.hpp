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

#ifndef BOOST_COROUTINES_NO_DEPRECATION_WARNING
// users should define this if they directly include boost/asio/spawn.hpp,
// but by defining it here, warnings won't cause problems with a compile
#define BOOST_COROUTINES_NO_DEPRECATION_WARNING
#endif

#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
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

    /** @brief Perform an asynchronous send of a message, executing the handler
     *         upon return and return
     *
     *  @param[in] m - A message ready to send
     *  @param[in] handler - handler to execute upon completion; this may be an
     *                       asio::yield_context to execute asynchronously as a
     *                       coroutine
     *
     *  @return If a yield context is passed as the handler, the return type is
     *          a message. If a function object is passed in as the handler,
     *          the return type is the result of the handler registration,
     *          while the resulting message will get passed into the handler.
     */
    template <typename MessageHandler>
    inline BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
                                         void(boost::system::error_code,
                                              message::message&))
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

    /** @brief Perform an asynchronous method call, with input parameter packing
     *         and return value unpacking
     *
     *  @param[in] handler - A function object that is to be called as a
     *                       continuation for the async dbus method call. The
     *                       arguments to parse on the return are deduced from
     *                       the handler's signature and then passed in along
     *                       with an error code.
     *  @param[in] service - The service to call.
     *  @param[in] objpath - The object's path for the call.
     *  @param[in] interf - The object's interface to call.
     *  @param[in] method - The object's method to call.
     *  @param[in] a... - Optional parameters for the method call.
     *
     *  @return immediate return of the internal handler registration. The
     *          result of the actual asynchronous call will get unpacked from
     *          the message and passed into the handler when the call is
     *          complete.
     */
    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        message::message m = new_method_call(service.c_str(), objpath.c_str(),
                                             interf.c_str(), method.c_str());
        m.append(a...);
        async_send(m, [handler](boost::system::error_code ec,
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

    /** @brief Perform a yielding asynchronous method call, with input
     *         parameter packing and return value unpacking
     *
     *  @param[in] yield - A yield context to async block upon. To catch errors
     *                     for the call, call this function with 'yield[ec]',
     *                     thus attaching an error code to the yield context
     *  @param[in] service - The service to call.
     *  @param[in] objpath - The object's path for the call.
     *  @param[in] interf - The object's interface to call.
     *  @param[in] method - The object's method to call.
     *  @param[in] a... - Optional parameters for the method call.
     *
     *  @return Unpacked value of RetType
     */
    template <typename... RetTypes, typename... InputArgs>
    auto yield_method_call(boost::asio::yield_context yield,
                           const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        message::message m = new_method_call(service.c_str(), objpath.c_str(),
                                             interf.c_str(), method.c_str());
        m.append(a...);
        message::message r = async_send(m, yield);
        if constexpr (sizeof...(RetTypes) == 0)
        {
            // void return
            return;
        }
        else if constexpr (sizeof...(RetTypes) == 1)
        {
            if constexpr (std::is_same<utility::first_type<RetTypes...>,
                                       void>::value)
            {
                return;
            }
            else
            {
                // single item return
                utility::first_type<RetTypes...> responseData;
                // this will throw if the signature of r != RetType
                r.read(responseData);
                return responseData;
            }
        }
        else
        {
            // tuple of things to return
            std::tuple<RetTypes...> responseData;
            // this will throw if the signature of r != RetType
            r.read(responseData);
            return responseData;
        }
    }

    boost::asio::io_service& get_io_service()
    {
        return io_;
    }

  private:
    boost::asio::io_service& io_;
    boost::asio::posix::stream_descriptor socket;

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
