// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_ASYNC_SEND_OP_HPP
#define DBUS_ASYNC_SEND_OP_HPP

#include <boost/scoped_ptr.hpp>

#include <dbus/dbus.h>
#include <dbus/error.hpp>
#include <dbus/message.hpp>

#include <dbus/impl/connection.ipp>

namespace dbus {
namespace detail {

template <typename MessageHandler>
struct async_send_op {
  boost::asio::io_service& io_;
  // TODO(ed) Instead of making message_ a unique_ptr, should probably split
  // async_send_op into 2 classes, one that expects a return, and one that does
  // not
  std::shared_ptr<message> message_;
  MessageHandler handler_;
  async_send_op(boost::asio::io_service& io,
                BOOST_ASIO_MOVE_ARG(MessageHandler) handler);
  static void callback(DBusPendingCall* p, void* userdata);  // for C API
  void operator()(impl::connection& c, message& m);  // initiate operation
  void operator()();  // bound completion handler form
};

template <typename MessageHandler>
async_send_op<MessageHandler>::async_send_op(boost::asio::io_service& io,
                                             BOOST_ASIO_MOVE_ARG(MessageHandler)
                                                 handler)
    : io_(io), handler_(BOOST_ASIO_MOVE_CAST(MessageHandler)(handler)) {}

template <typename MessageHandler>
void async_send_op<MessageHandler>::operator()(impl::connection& c,
                                               message& m) {
  DBusPendingCall* p;
  if (m.get_type() != "method_call") {
    // For some undocumented reason, dbus_connection_send_with_reply doesn't
    // work when the message type is signal.  To avoid a memory leak on the
    // dbuspending call when the callback doesn't fire, simply send it without a
    // reply
    c.send(m);
  } else {
    c.send_with_reply(m, &p, -1);

    // We have to throw this onto the heap so that the
    // C API can store it as `void *userdata`
    async_send_op* op =
        new async_send_op(BOOST_ASIO_MOVE_CAST(async_send_op)(*this));
    // dbus_pending_call_unref(p);

    dbus_pending_call_set_notify(p, &callback, op, NULL);

    // FIXME Race condition: another thread might have
    // processed the pending call's reply before a notify
    // function could be set. If so, the notify function
    // will never trigger, so it must be called manually:
    if (dbus_pending_call_get_completed(p)) {
      // TODO: does this work, or might it call the notify
      // function too many times? Might have to use steal_reply
      // callback(p, op);
    }
  }
}

template <typename MessageHandler>
void async_send_op<MessageHandler>::callback(DBusPendingCall* p,
                                             void* userdata) {
  boost::scoped_ptr<async_send_op> op(static_cast<async_send_op*>(userdata));
  auto x = dbus_pending_call_steal_reply(p);
  op->message_ = std::make_shared<message>(x);
  dbus_message_unref(x);
  dbus_pending_call_unref(p);

  op->io_.post(BOOST_ASIO_MOVE_CAST(async_send_op)(*op));
}

template <typename MessageHandler>
void async_send_op<MessageHandler>::operator()() {
  handler_(error(*message_.get()).error_code(), *message_.get());
}

}  // namespace detail
}  // namespace dbus

#endif  // DBUS_ASYNC_SEND_OP_HPP
