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
#include <sdbusplus/asio/detail/async_send_handler.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/read_into_tuple.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <chrono>
#include <string>
#include <tuple>

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
    connection(boost::asio::io_context& io) :
        sdbusplus::bus::bus(sdbusplus::bus::new_default()), io_(io), socket(io_)
    {
        socket.assign(get_fd());
        read_wait();
    }
    connection(boost::asio::io_context& io, sd_bus* bus) :
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
        async_send(message::message& m, MessageHandler&& handler,
                   uint64_t timeout = 0)
    {
        boost::asio::async_completion<
            MessageHandler, void(boost::system::error_code, message::message)>
            init(handler);
        detail::async_send_handler<typename boost::asio::async_result<
            MessageHandler, void(boost::system::error_code,
                                 message::message)>::completion_handler_type>(
            std::move(init.completion_handler))(get(), m, timeout);
        return init.result.get();
    }

    /** @brief Perform an asynchronous method call, with input parameter packing
     *         and return value unpacking.
     *
     *  @param[in] handler - A function object that is to be called as a
     *                       continuation for the async dbus method call. The
     *                       arguments to parse on the return are deduced from
     *                       the handler's signature and then passed in along
     *                       with an error code and optional message::message
     *  @param[in] service - The service to call.
     *  @param[in] objpath - The object's path for the call.
     *  @param[in] interf - The object's interface to call.
     *  @param[in] method - The object's method to call.
     *  @param[in] timeout - The timeout for the method call in usec (0 results
     *                       in using the default value).
     *  @param[in] a... - Optional parameters for the method call.
     *
     *  @return immediate return of the internal handler registration. The
     *          result of the actual asynchronous call will get unpacked from
     *          the message and passed into the handler when the call is
     *          complete.
     */
    template <typename MessageHandler, typename... InputArgs>
    void async_method_call_timed(MessageHandler&& handler,
                                 const std::string& service,
                                 const std::string& objpath,
                                 const std::string& interf,
                                 const std::string& method, uint64_t timeout,
                                 const InputArgs&... a)
    {
        using FunctionTuple = boost::callable_traits::args_t<MessageHandler>;
        using FunctionTupleType =
            typename utility::decay_tuple<FunctionTuple>::type;
        constexpr bool returnWithMsg = []() {
            if constexpr (std::tuple_size_v<FunctionTupleType> > 1)
            {
                return std::is_same_v<
                    std::tuple_element_t<1, FunctionTupleType>,
                    sdbusplus::message::message>;
            }
            return false;
        }();
        using UnpackType =
            typename utility::strip_first_n_args<returnWithMsg ? 2 : 1,
                                                 FunctionTupleType>::type;
        auto applyHandler = [handler = std::forward<MessageHandler>(handler)](
                                boost::system::error_code ec,
                                message::message& r) mutable {
            UnpackType responseData;
            if (!ec)
            {
                try
                {
                    utility::read_into_tuple(responseData, r);
                }
                catch (const std::exception& e)
                {
                    // Set error code if not already set
                    ec = boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument);
                }
            }
            // Note.  Callback is called whether or not the unpack was
            // successful to allow the user to implement their own handling
            if constexpr (returnWithMsg)
            {
                auto response = std::tuple_cat(std::make_tuple(ec),
                                               std::forward_as_tuple(r),
                                               std::move(responseData));
                std::apply(handler, response);
            }
            else
            {
                auto response = std::tuple_cat(std::make_tuple(ec),
                                               std::move(responseData));
                std::apply(handler, response);
            }
        };
        message::message m;
        boost::system::error_code ec;
        try
        {
            m = new_method_call(service.c_str(), objpath.c_str(),
                                interf.c_str(), method.c_str());
            m.append(a...);
        }
        catch (const exception::SdBusError& e)
        {
            ec = boost::system::errc::make_error_code(
                static_cast<boost::system::errc::errc_t>(e.get_errno()));
            applyHandler(ec, m);
            return;
        }
        async_send(m, std::forward<decltype(applyHandler)>(applyHandler),
                   timeout);
    }

    /** @brief Perform an asynchronous method call, with input parameter packing
     *         and return value unpacking. Uses the default timeout value.
     *
     *  @param[in] handler - A function object that is to be called as a
     *                       continuation for the async dbus method call. The
     *                       arguments to parse on the return are deduced from
     *                       the handler's signature and then passed in along
     *                       with an error code and optional message::message
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
    void async_method_call(MessageHandler&& handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        async_method_call_timed(std::forward<MessageHandler>(handler), service,
                                objpath, interf, method, 0, a...);
    }

    /** @brief Perform a yielding asynchronous method call, with input
     *         parameter packing and return value unpacking
     *
     *  @param[in] yield - A yield context to async block upon.
     *  @param[in] ec - an error code that will be set for any errors
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
                           boost::system::error_code& ec,
                           const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           const InputArgs&... a)
    {
        message::message m;
        try
        {
            m = new_method_call(service.c_str(), objpath.c_str(),
                                interf.c_str(), method.c_str());
            m.append(a...);
        }
        catch (const exception::SdBusError& e)
        {
            ec = boost::system::errc::make_error_code(
                static_cast<boost::system::errc::errc_t>(e.get_errno()));
        }
        message::message r;
        if (!ec)
        {
            r = async_send(m, yield[ec]);
        }
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
                utility::first_type<RetTypes...> responseData{};
                // before attempting to read, check ec and bail on error
                if (ec)
                {
                    return responseData;
                }
                try
                {
                    r.read(responseData);
                }
                catch (const std::exception& e)
                {
                    ec = boost::system::errc::make_error_code(
                        boost::system::errc::invalid_argument);
                    // responseData will be default-constructed...
                }
                return responseData;
            }
        }
        else
        {
            // tuple of things to return
            std::tuple<RetTypes...> responseData{};
            // before attempting to read, check ec and bail on error
            if (ec)
            {
                return responseData;
            }
            try
            {
                r.read(responseData);
            }
            catch (const std::exception& e)
            {
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
                // responseData will be default-constructed...
            }
            return responseData;
        }
    }

    boost::asio::io_context& get_io_context()
    {
        return io_;
    }

  private:
    boost::asio::io_context& io_;
    boost::asio::posix::stream_descriptor socket;

    void read_wait()
    {
        socket.async_read_some(
            boost::asio::null_buffers(),
            [&](const boost::system::error_code& /*ec*/, std::size_t) {
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
        boost::asio::post(io_, [&] {
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
