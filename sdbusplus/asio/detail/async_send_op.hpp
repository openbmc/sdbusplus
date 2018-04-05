// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <boost/scoped_ptr.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/bus.hpp>
#include <systemd/sd-bus.h>
#include <boost/asio.hpp>

namespace sdbusplus
{
namespace asio
{
namespace detail
{

template <typename MessageHandler> struct async_send_op
{
    boost::asio::io_service& io_;
    std::shared_ptr<message::message> message_;
    MessageHandler handler_;
    async_send_op(boost::asio::io_service& io, MessageHandler&& handler);
    static int callback(sd_bus_message* m, void* userdata,
                        sd_bus_error* ret_error); // for C API
    void operator()(sd_bus* c,
                    message::message& m); // initiate operation
    void operator()();                    // bound completion handler form
};

template <typename MessageHandler>
async_send_op<MessageHandler>::async_send_op(boost::asio::io_service& io,
                                             MessageHandler&& handler) :
    io_(io),
    handler_(std::move(handler))
{
}

template <typename MessageHandler>
void async_send_op<MessageHandler>::operator()(sd_bus* conn,
                                               message::message& m)
{

    // We have to throw this onto the heap so that the
    // C API can store it as `void *userdata`
    async_send_op* op = new async_send_op(std::move(*this));

    sd_bus_message* dbusMessage = m.release();

    // 0 is default timeout
    if (sd_bus_call_async(conn, NULL, dbusMessage, &callback, op, 0) < 0)
    {
        throw std::runtime_error("DBus method call failure\n");
    }
}

template <typename MessageHandler>
int async_send_op<MessageHandler>::callback(sd_bus_message* m, void* userdata,
                                            sd_bus_error* ret_error)
{
    if (userdata == nullptr || m == nullptr)
    {
        return -1;
    }
    std::unique_ptr<async_send_op> op(static_cast<async_send_op*>(userdata));

    op->message_ = std::make_shared<message::message>(m);
    op->io_.post(std::move(*op));
    return 0;
}
template <typename MessageHandler>
void async_send_op<MessageHandler>::operator()()
{
    auto ec = make_error_code(message_->is_method_error()
                                  ? boost::system::errc::no_message
                                  : boost::system::errc::success);
    handler_(ec, *(message_.get()));
}

} // namespace detail
} // namespace asio
} // namespace sdbusplus
