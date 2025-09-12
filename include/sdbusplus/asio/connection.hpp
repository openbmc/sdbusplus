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

#include <boost/asio/async_result.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>

#ifndef SDBUSPLUS_DISABLE_BOOST_COROUTINES
#include <boost/asio/spawn.hpp>
#endif
#include <boost/callable_traits.hpp>
#include <sdbusplus/asio/detail/async_send_handler.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <chrono>
#include <functional>
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
class connection : public sdbusplus::bus_t
{
  public:
    // default to default bus
    explicit connection(
        boost::asio::io_context& io,
        sdbusplus::bus_t&& bus = sdbusplus::bus::new_default()) :
        sdbusplus::bus_t(std::move(bus)), io_(io),
        socket(io_.get_executor(), get_fd()), timer(io_.get_executor())
    {
        read_immediate();
    }
    connection(boost::asio::io_context& io, sd_bus* bus) :
        sdbusplus::bus_t(bus), io_(io), socket(io_.get_executor(), get_fd()),
        timer(io_.get_executor())
    {
        read_immediate();
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
     *  @param[in] token - The completion token to execute upon completion;
     *  @param[in] timeout - The timeout in microseconds
     *
     */

    using callback_t = void(boost::system::error_code, message_t&);
    using send_function = std::move_only_function<callback_t>;
    inline void async_send(message_t& m, send_function&& callback,
                           uint64_t timeout = 0)
    {
        boost::asio::async_initiate<send_function, callback_t>(
            detail::async_send_handler(get(), m, timeout), callback);
    }
#ifndef SDBUSPLUS_DISABLE_BOOST_COROUTINES
    inline auto async_send_yield(message_t& m,
                                 boost::asio::yield_context&& token,
                                 uint64_t timeout = 0)
    {
        using yield_callback_t = void(boost::system::error_code, message_t);
        return boost::asio::async_initiate<boost::asio::yield_context,
                                           yield_callback_t>(
            detail::async_send_handler(get(), m, timeout), token);
    }
#endif

    template <typename MessageHandler>
    static void unpack(boost::system::error_code ec, message_t& r,
                       MessageHandler&& handler)
    {
        using FunctionTuple = boost::callable_traits::args_t<MessageHandler>;
        using FunctionTupleType = utility::decay_tuple_t<FunctionTuple>;
        constexpr bool returnWithMsg = []() {
            if constexpr ((std::tuple_size_v<FunctionTupleType>) > 1)
            {
                return std::is_same_v<
                    std::tuple_element_t<1, FunctionTupleType>,
                    sdbusplus::message_t>;
            }
            return false;
        }();
        using UnpackType = utility::strip_first_n_args_t<returnWithMsg ? 2 : 1,
                                                         FunctionTupleType>;
        UnpackType responseData;
        if (!ec)
        {
            try
            {
                std::apply([&r](auto&... x) { r.read(x...); }, responseData);
            }
            catch (const std::exception&)
            {
                // Set error code if not already set
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
            }
        }
        // Note.  Callback is called whether or not the unpack was
        // successful to allow the user to implement their own
        // handling
        if constexpr (returnWithMsg)
        {
            auto response =
                std::tuple_cat(std::make_tuple(ec), std::forward_as_tuple(r),
                               std::move(responseData));
            std::apply(handler, response);
        }
        else
        {
            auto response =
                std::tuple_cat(std::make_tuple(ec), std::move(responseData));
            std::apply(handler, response);
        }
    }

    /** @brief Perform an asynchronous method call, with input parameter packing
     *         and return value unpacking.
     *
     *  @param[in] handler - A function object that is to be called as a
     *                       continuation for the async dbus method call. The
     *                       arguments to parse on the return are deduced from
     *                       the handler's signature and then passed in along
     *                       with an error code and optional message_t
     *  @param[in] service - The service to call.
     *  @param[in] objpath - The object's path for the call.
     *  @param[in] interf - The object's interface to call.
     *  @param[in] method - The object's method to call.
     *  @param[in] timeout - The timeout for the method call in usec (0 results
     *                       in using the default value).
     *  @param[in] a - Optional parameters for the method call.
     *
     */
    template <typename MessageHandler, typename... InputArgs>
    void async_method_call_timed(
        MessageHandler&& handler, const std::string& service,
        const std::string& objpath, const std::string& interf,
        const std::string& method, uint64_t timeout, const InputArgs&... a)
    {
        using callback_t = std::move_only_function<void(
            boost::system::error_code, message_t&)>;
        callback_t applyHandler =
            [handler = std::forward<MessageHandler>(
                 handler)](boost::system::error_code ec, message_t& r) mutable {
                unpack(ec, r, std::move(handler));
            };
        message_t m;
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
        async_send(m, std::move(applyHandler), timeout);
    }

    /** @brief Perform an asynchronous method call, with input parameter packing
     *         and return value unpacking. Uses the default timeout value.
     *
     *  @param[in] handler - A function object that is to be called as a
     *                       continuation for the async dbus method call. The
     *                       arguments to parse on the return are deduced from
     *                       the handler's signature and then passed in along
     *                       with an error code and optional message_t
     *  @param[in] service - The service to call.
     *  @param[in] objpath - The object's path for the call.
     *  @param[in] interf - The object's interface to call.
     *  @param[in] method - The object's method to call.
     *  @param[in] a - Optional parameters for the method call.
     *
     */
    template <typename MessageHandler, typename... InputArgs>
    void async_method_call(MessageHandler&& handler, const std::string& service,
                           const std::string& objpath,
                           const std::string& interf, const std::string& method,
                           InputArgs&&... a)
    {
        async_method_call_timed(std::forward<MessageHandler>(handler), service,
                                objpath, interf, method, 0,
                                std::forward<InputArgs>(a)...);
    }

#ifndef SDBUSPLUS_DISABLE_BOOST_COROUTINES
    template <typename... RetTypes>
    auto default_ret_types()
    {
        if constexpr (sizeof...(RetTypes) == 0)
        {
            return;
        }
        else if constexpr (sizeof...(RetTypes) == 1 &&
                           std::is_void_v<std::tuple_element_t<
                               0, std::tuple<RetTypes...>>>)
        {
            return;
        }
        else if constexpr (sizeof...(RetTypes) == 1)
        {
            return std::tuple_element_t<0, std::tuple<RetTypes...>>{};
        }
        else
        {
            return std::tuple<RetTypes...>{};
        }
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
     *  @param[in] a - Optional parameters for the method call.
     *
     *  @return Unpacked value of RetType
     */
    template <typename... RetTypes, typename... InputArgs>
    auto yield_method_call(
        boost::asio::yield_context yield, boost::system::error_code& ec,
        const std::string& service, const std::string& objpath,
        const std::string& interf, const std::string& method,
        const InputArgs&... a)
    {
        message_t m;
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
        if (!ec)
        {
            message_t r;
            r = async_send_yield(m, yield[ec]);
            try
            {
                return r.unpack<RetTypes...>();
            }
            catch (const std::exception&)
            {
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
            }
        }
        return default_ret_types<RetTypes...>();
    }
#endif
    boost::asio::io_context& get_io_context()
    {
        return io_;
    }

  private:
    boost::asio::io_context& io_;
    boost::asio::posix::stream_descriptor socket;
    boost::asio::steady_timer timer;

    void process()
    {
        if (process_discard())
        {
            read_immediate();
        }
        else
        {
            read_wait();
        }
    }

    void on_fd_event(const boost::system::error_code& ec)
    {
        // This is expected if the timer expired before an fd event was
        // available
        if (ec == boost::asio::error::operation_aborted)
        {
            return;
        }
        timer.cancel();
        if (ec)
        {
            return;
        }
        process();
    }

    void on_timer_event(const boost::system::error_code& ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // This is expected if the fd was available before the timer expired
            return;
        }
        if (ec)
        {
            return;
        }
        // Abort existing operations on the socket
        socket.cancel();
        process();
    }

    void read_wait()
    {
        int fd = get_fd();
        if (fd < 0)
        {
            return;
        }
        if (fd != socket.native_handle())
        {
            socket.release();
            socket.assign(fd);
        }
        int events = get_events();
        if (events < 0)
        {
            return;
        }
        if (events & POLLIN)
        {
            socket.async_wait(boost::asio::posix::stream_descriptor::wait_read,
                              std::bind_front(&connection::on_fd_event, this));
        }
        if (events & POLLOUT)
        {
            socket.async_wait(boost::asio::posix::stream_descriptor::wait_write,
                              std::bind_front(&connection::on_fd_event, this));
        }
        if (events & POLLERR)
        {
            socket.async_wait(boost::asio::posix::stream_descriptor::wait_error,
                              std::bind_front(&connection::on_fd_event, this));
        }

        uint64_t timeout = 0;
        int timeret = get_timeout(&timeout);
        if (timeret < 0)
        {
            return;
        }
        using clock = std::chrono::steady_clock;

        using SdDuration = std::chrono::duration<uint64_t, std::micro>;
        SdDuration sdTimeout(timeout);
        // sd-bus always returns a 64 bit timeout regardless of architecture,
        // and per the documentation routinely returns UINT64_MAX
        if (sdTimeout > clock::duration::max())
        {
            // No need to start the timer if the expiration is longer than
            // underlying timer can run.
            return;
        }
        auto nativeTimeout = std::chrono::floor<clock::duration>(sdTimeout);
        timer.expires_at(clock::time_point(nativeTimeout));
        timer.async_wait(std::bind_front(&connection::on_timer_event, this));
    }
    void read_immediate()
    {
        boost::asio::post(io_, std::bind_front(&connection::process, this));
    }
};

} // namespace asio

} // namespace sdbusplus
