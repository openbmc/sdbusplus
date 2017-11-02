// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_CONNECTION_HPP
#define DBUS_CONNECTION_HPP

#include <dbus/connection_service.hpp>
#include <dbus/element.hpp>
#include <dbus/message.hpp>
#include <chrono>
#include <string>
#include <boost/asio.hpp>

namespace dbus {

class filter;
class match;

/// Root D-Bus IO object
/**
 * A connection to a bus, through which messages may be sent or received.
 */
class connection : public boost::asio::basic_io_object<connection_service> {
 public:
  /// Open a connection to a specified address.
  /**
 * @param io_service The io_service object that the connection will use to
 * wire D-Bus for asynchronous operation.
 *
 * @param address The address of the bus to connect to.
 *
 * @throws boost::system::system_error When opening the connection failed.
 */
  connection(boost::asio::io_service& io, const string& address)
      : basic_io_object<connection_service>(io) {
    this->get_service().open(this->get_implementation(), address);
  }

  /// Open a connection to a well-known bus.
  /**
 * D-Bus connections are usually opened to well-known buses like the
 * system or session bus.
 *
 * @param bus The well-known bus to connect to.
 *
 * @throws boost::system::system_error When opening the connection failed.
 */
  // TODO: change this unsigned to an enumeration
  connection(boost::asio::io_service& io, const int bus)
      : basic_io_object<connection_service>(io) {
    this->get_service().open(this->get_implementation(), bus);
  }

  /// Request a name on the bus.
  /**
 * @param name The name requested on the bus
 *
 * @return
 *
 * @throws boost::system::system_error When the response timed out or
 * there was some other error.
 */
  void request_name(const string& name) {
    this->get_implementation().request_name(name);
  }

  std::string get_unique_name() {
    return this->get_implementation().get_unique_name();
  }

  /// Reply to a message.
  /**
 * @param m The message from which to create the reply
 *
 * @return The new reply message
 *
 * @throws boost::system::system_error When the response timed out or
 * there was some other error.
 */
  message reply(message& m) {
    return this->get_implementation().new_method_return(m);
  }

  /// Send a message.
  /**
 * @param m The message to send.
 *
 * @return The reply received.
 *
 * @throws boost::system::system_error When the response timed out or
 * there was some other error.
 */
  message send(message& m) {
    return this->get_service().send(this->get_implementation(), m);
  }

  /// Send a message.
  /**
 * @param m The message to send.
 *
 * @param t Time to wait for a reply. Passing 0 as the timeout means
 * that you wish to ignore the reply. (Or catch it later somehow...)
 *
 * @return The reply received.
 *
 * @throws boost::system::system_error When the response timed out (if
 * timeout was not 0), or there was some other error.
 */
  template <typename Duration>
  message send(message& m, const Duration& t) {
    return this->get_service().send(this->get_implementation(), m, t);
  }

  template <typename... InputArgs>
  message method_call(const dbus::endpoint& e, const InputArgs&... a) {
    message m = dbus::message::new_call(e);
    m.pack(a...);
    return this->get_service().send(this->get_implementation(), m,
                                    std::chrono::seconds(30));
  }

  /// Send a message asynchronously.
  /**
 * @param m The message to send.
 *
 * @param handler Handler for the reply.
 *
 * @return Asynchronous result
 */

  template <typename MessageHandler>
  inline BOOST_ASIO_INITFN_RESULT_TYPE(MessageHandler,
                                       void(boost::system::error_code, message))
      async_send(message& m, BOOST_ASIO_MOVE_ARG(MessageHandler) handler) {
    return this->get_service().async_send(
        this->get_implementation(), m,
        BOOST_ASIO_MOVE_CAST(MessageHandler)(handler));
  }

  // Small helper class for stipping off the error code from the function
  // agrument definitions so unpack can be called appriately
  template <typename T>
  struct strip_first_arg {};

  template <typename FirstArg, typename... Rest>
  struct strip_first_arg<std::tuple<FirstArg, Rest...>> {
    typedef std::tuple<Rest...> type;
  };

  template <typename MessageHandler, typename... InputArgs>
  auto async_method_call(MessageHandler handler, const dbus::endpoint& e,
                         const InputArgs&... a) {
    message m = dbus::message::new_call(e);
    if (!m.pack(a...)) {
      // TODO(ed) Set error code?
      std::cerr << "Pack error\n";
    } else {
      // explicit copy of handler here.  At this time, not clear why a copy is
      // needed
      return async_send(
          m, [handler](boost::system::error_code ec, dbus::message r) {
            // Make a copy of the error code so we can modify it
            typedef typename function_traits<MessageHandler>::decayed_arg_types
                function_tuple;
            typedef typename strip_first_arg<function_tuple>::type unpack_type;
            unpack_type response_args;
            if (!ec) {
              if (!unpack_into_tuple(response_args, r)) {
                // Set error code
                ec = boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);
              }
            }
            // Should this (the callback) be done in a try catch block?
            // should throwing from a handler flow all the way to the
            // io_service?

            // Note.  Callback is called whether or not the unpack was sucessful
            // to allow the user to implement their own handling
            index_apply<std::tuple_size<unpack_type>{}>([&](auto... Is) {
              handler(ec, std::get<Is>(response_args)...);
            });
          });
    }
  }

  /// Create a new match.
  void new_match(match& m) {
    this->get_service().new_match(this->get_implementation(), m);
  }

  /// Destroy a match.
  void delete_match(match& m) {
    this->get_service().delete_match(this->get_implementation(), m);
  }

  /// Create a new filter.
  void new_filter(filter& f) {
    this->get_service().new_filter(this->get_implementation(), f);
  }

  /// Destroy a filter.
  void delete_filter(filter& f) {
    this->get_service().delete_filter(this->get_implementation(), f);
  }

  // FIXME the only way around this I see is to expose start() here, which seems
  // ugly
  friend class filter;
};

typedef std::shared_ptr<connection> connection_ptr;

}  // namespace dbus

#endif  // DBUS_CONNECTION_HPP
