// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_MESSAGE_HPP
#define DBUS_MESSAGE_HPP

#include <dbus/dbus.h>
#include <dbus/element.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/impl/message_iterator.hpp>
#include <iostream>
#include <vector>
#include <boost/intrusive_ptr.hpp>
#include <boost/mpl/for_each.hpp>
#include <boost/utility/enable_if.hpp>

inline void intrusive_ptr_add_ref(DBusMessage* m) { dbus_message_ref(m); }

inline void intrusive_ptr_release(DBusMessage* m) { dbus_message_unref(m); }

namespace dbus {

class message {
 private:
  boost::intrusive_ptr<DBusMessage> message_;

 public:
  /// Create a method call message
  static message new_call(const endpoint& destination) {
    auto x = message(dbus_message_new_method_call(
        destination.get_process_name().c_str(), destination.get_path().c_str(),
        destination.get_interface().c_str(), destination.get_member().c_str()));
    dbus_message_unref(x.message_.get());
    return x;
  }

  /// Create a method call message
  static message new_call(const endpoint& destination,
                          const string& method_name) {
    auto x = message(dbus_message_new_method_call(
        destination.get_process_name().c_str(), destination.get_path().c_str(),
        destination.get_interface().c_str(), method_name.c_str()));
    dbus_message_unref(x.message_.get());
    return x;
  }

  /// Create a method return message
  static message new_return(message& call) {
    auto x = message(dbus_message_new_method_return(call));
    dbus_message_unref(x.message_.get());
    return x;
  }

  /// Create an error message
  static message new_error(message& call, const string& error_name,
                           const string& error_message) {
    auto x = message(dbus_message_new_error(call, error_name.c_str(),
                                            error_message.c_str()));
    dbus_message_unref(x.message_.get());
    return x;
  }

  /// Create a signal message
  static message new_signal(const endpoint& origin, const string& signal_name) {
    auto x = message(dbus_message_new_signal(origin.get_path().c_str(),
                                             origin.get_interface().c_str(),
                                             signal_name.c_str()));
    dbus_message_unref(x.message_.get());
    return x;
  }

  message() = delete;

  message(DBusMessage* m) : message_(m) {}

  operator DBusMessage*() { return message_.get(); }

  operator const DBusMessage*() const { return message_.get(); }

  string get_path() const {
    return sanitize(dbus_message_get_path(message_.get()));
  }

  string get_interface() const {
    return sanitize(dbus_message_get_interface(message_.get()));
  }

  string get_member() const {
    return sanitize(dbus_message_get_member(message_.get()));
  }

  string get_type() const {
    return sanitize(
        dbus_message_type_to_string(dbus_message_get_type(message_.get())));
  }

  string get_signature() const {
    return sanitize(dbus_message_get_signature(message_.get()));
  }

  string get_sender() const {
    return sanitize(dbus_message_get_sender(message_.get()));
  }

  string get_destination() const {
    return sanitize(dbus_message_get_destination(message_.get()));
  }

  uint32 get_serial() { return dbus_message_get_serial(message_.get()); }

  message& set_serial(uint32 serial) {
    dbus_message_set_serial(message_.get(), serial);
    return *this;
  }

  uint32 get_reply_serial() {
    return dbus_message_get_reply_serial(message_.get());
  }

  message& set_reply_serial(uint32 reply_serial) {
    dbus_message_set_reply_serial(message_.get(), reply_serial);
    return *this;
  }

  struct packer {
    impl::message_iterator iter_;
    packer(message& m) { impl::message_iterator::init_append(m, iter_); }
    packer(){};

    template <typename Element, typename... Args>
    bool pack(const Element& e, const Args&... args) {
      if (this->pack(e) == false) {
        return false;
      } else {
        return pack(args...);
      }
    }

    template <typename Element>
    typename boost::enable_if<is_fixed_type<Element>, bool>::type pack(
        const Element& e) {
      return iter_.append_basic(element<Element>::code, &e);
    }

    template <typename Element>
    typename boost::enable_if<std::is_pointer<Element>, bool>::type pack(
        const Element e) {
      return iter_.append_basic(
          element<typename std::remove_pointer<Element>::type>::code, e);
    }

    template <typename Container>
    typename std::enable_if<has_const_iterator<Container>::value &&
                                !is_string_type<Container>::value,
                            bool>::type
    pack(const Container& c) {
      message::packer sub;

      static const constexpr auto signature =
          element_signature<Container>::code;
      if (iter_.open_container(signature[0], &signature[1], sub.iter_) ==
          false) {
        return false;
      }
      for (auto& element : c) {
        if (!sub.pack(element)) {
          return false;
        }
      }
      return iter_.close_container(sub.iter_);
    }

    bool pack(const char* c) {
      return iter_.append_basic(element<string>::code, &c);
    }

    // bool pack specialization
    bool pack(bool c) {
      int v = c;
      return iter_.append_basic(element<bool>::code, &v);
    }

    template <typename Key, typename Value>
    bool pack(const std::pair<Key, Value> element) {
      message::packer dict_entry;
      if (iter_.open_container(DBUS_TYPE_DICT_ENTRY, NULL, dict_entry.iter_) ==
          false) {
        return false;
      }
      if (dict_entry.pack(element.first) == false) {
        return false;
      };
      if (dict_entry.pack(element.second) == false) {
        return false;
      };
      return iter_.close_container(dict_entry.iter_);
    }

    bool pack(const object_path& e) {
      const char* c = e.value.c_str();
      return iter_.append_basic(element<object_path>::code, &c);
    }

    bool pack(const string& e) {
      const char* c = e.c_str();
      return pack(c);
    }

    bool pack(const dbus_variant& v) {
      // Get the dbus typecode  of the variant being packed
      const char* type = boost::apply_visitor(
          [&](auto val) {
            static const constexpr auto sig =
                element_signature<decltype(val)>::code;
            static_assert(
                std::tuple_size<decltype(sig)>::value == 2,
                "Element signature for dbus_variant too long.  Expected "
                "length of 1");
            return &sig[0];
          },
          v);
      message::packer sub;
      iter_.open_container(element<dbus_variant>::code, type, sub.iter_);
      boost::apply_visitor([&](const auto& val) { sub.pack(val); }, v);
      iter_.close_container(sub.iter_);

      return true;
    }
  };

  template <typename... Args>
  bool pack(const Args&... args) {
    return packer(*this).pack(args...);
  }

  // noop for template functions that have no arguments
  bool pack() { return true; }

  struct unpacker {
    impl::message_iterator iter_;
    unpacker(message& m) { impl::message_iterator::init(m, iter_); }
    unpacker() {}

    template <typename Element, typename... Args>
    bool unpack(Element& e, Args&... args) {
      if (unpack(e) == false) {
        return false;
      }
      return unpack(args...);
    }

    // Basic type unpack
    template <typename Element>
    typename boost::enable_if<is_fixed_type<Element>, bool>::type unpack(
        Element& e) {
      if (iter_.get_arg_type() != element<Element>::code) {
        return false;
      }
      iter_.get_basic(&e);
      // ignoring return code here, as we might hit last element, and don't
      // really care because get_arg_type will return invalid if we call it
      // after we're over the struct boundary
      iter_.next();
      return true;
    }

    // bool unpack specialization
    bool unpack(bool& s) {
      if (iter_.get_arg_type() != element<bool>::code) {
        return false;
      }
      int c;
      iter_.get_basic(&c);
      s = c;
      iter_.next();
      return true;
    }

    // std::string unpack specialization
    bool unpack(string& s) {
      if (iter_.get_arg_type() != element<string>::code) {
        return false;
      }
      const char* c;
      iter_.get_basic(&c);
      s.assign(c);
      iter_.next();
      return true;
    }

    // object_path unpack specialization
    bool unpack(object_path& s) {
      if (iter_.get_arg_type() != element<object_path>::code) {
        return false;
      }
      const char* c;
      iter_.get_basic(&c);
      s.value.assign(c);
      iter_.next();
      return true;
    }

    // object_path unpack specialization
    bool unpack(signature& s) {
      if (iter_.get_arg_type() != element<signature>::code) {
        return false;
      }
      const char* c;
      iter_.get_basic(&c);
      s.value.assign(c);
      iter_.next();
      return true;
    }

    // variant unpack specialization
    bool unpack(dbus_variant& v) {
      if (iter_.get_arg_type() != element<dbus_variant>::code) {
        return false;
      }
      message::unpacker sub;
      iter_.recurse(sub.iter_);

      char arg_type = sub.iter_.get_arg_type();

      boost::mpl::for_each<dbus_variant::types>([&](auto t) {
        if (arg_type == element<decltype(t)>::code) {
          decltype(t) val_to_fill;
          sub.unpack(val_to_fill);
          v = val_to_fill;
        }
      });

      iter_.next();
      return true;
    }

    // dict entry unpack specialization
    template <typename Key, typename Value>
    bool unpack(std::pair<Key, Value>& v) {
      auto this_code = iter_.get_arg_type();
      // This can't use element<std::pair> because there is a difference between
      // the dbus type code 'e' and the dbus signature code for dict entries
      // '{'.  Element_signature will return the full signature, and will never
      // return e, but we still want to type check it before recursing
      if (this_code != DBUS_TYPE_DICT_ENTRY) {
        return false;
      }
      message::unpacker sub;
      iter_.recurse(sub.iter_);
      if (!sub.unpack(v.first)) {
        return false;
      }
      if (!sub.unpack(v.second)) {
        return false;
      }

      iter_.next();
      return true;
    }

    template <typename Container>
    typename std::enable_if<has_const_iterator<Container>::value &&
                                !is_string_type<Container>::value,
                            bool>::type
    unpack(Container& c) {
      auto top_level_arg_type = iter_.get_arg_type();
      constexpr auto type = element_signature<Container>::code[0];
      if (top_level_arg_type != type) {
        return false;
      }
      message::unpacker sub;

      iter_.recurse(sub.iter_);
      auto arg_type = sub.iter_.get_arg_type();
      while (arg_type != DBUS_TYPE_INVALID) {
        c.emplace_back();
        if (!sub.unpack(c.back())) {
          return false;
        }
        arg_type = sub.iter_.get_arg_type();
      }
      iter_.next();
      return true;
    }
  };

  template <typename... Args>
  bool unpack(Args&... args) {
    return unpacker(*this).unpack(args...);
  }

 private:
  static std::string sanitize(const char* str) {
    return (str == NULL) ? "(null)" : str;
  }
};

inline std::ostream& operator<<(std::ostream& os, const message& m) {
  os << "type='" << m.get_type() << "',"
     << "sender='" << m.get_sender() << "',"
     << "interface='" << m.get_interface() << "',"
     << "member='" << m.get_member() << "',"
     << "path='" << m.get_path() << "',"
     << "destination='" << m.get_destination() << "'";
  return os;
}

}  // namespace dbus

// primary template.
template <class T>
struct function_traits : function_traits<decltype(&T::operator())> {};

// partial specialization for function type
template <class R, class... Args>
struct function_traits<R(Args...)> {
  using result_type = R;
  using argument_types = std::tuple<Args...>;
  using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

// partial specialization for function pointer
template <class R, class... Args>
struct function_traits<R (*)(Args...)> {
  using result_type = R;
  using argument_types = std::tuple<Args...>;
  using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

// partial specialization for std::function
template <class R, class... Args>
struct function_traits<std::function<R(Args...)>> {
  using result_type = R;
  using argument_types = std::tuple<Args...>;
  using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

// partial specialization for pointer-to-member-function (i.e., operator()'s)
template <class T, class R, class... Args>
struct function_traits<R (T::*)(Args...)> {
  using result_type = R;
  using argument_types = std::tuple<Args...>;
  using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

template <class T, class R, class... Args>
struct function_traits<R (T::*)(Args...) const> {
  using result_type = R;
  using argument_types = std::tuple<Args...>;
  using decayed_arg_types = std::tuple<typename std::decay<Args>::type...>;
};

template <class F, size_t... Is>
constexpr auto index_apply_impl(F f, std::index_sequence<Is...>) {
  return f(std::integral_constant<size_t, Is>{}...);
}

template <size_t N, class F>
constexpr auto index_apply(F f) {
  return index_apply_impl(f, std::make_index_sequence<N>{});
}

template <class Tuple, class F>
constexpr auto apply(F f, Tuple& t) {
  return index_apply<std::tuple_size<Tuple>{}>(
      [&](auto... Is) { return f(std::get<Is>(t)...); });
}

template <class Tuple>
constexpr bool unpack_into_tuple(Tuple& t, dbus::message& m) {
  return index_apply<std::tuple_size<Tuple>{}>(
      [&](auto... Is) { return m.unpack(std::get<Is>(t)...); });
}

// Specialization for empty tuples.  No need to unpack if no arguments
constexpr bool unpack_into_tuple(std::tuple<>& t, dbus::message& m) {
  return true;
}

template <typename... Args>
constexpr bool pack_tuple_into_msg(std::tuple<Args...>& t, dbus::message& m) {
  return index_apply<std::tuple_size<std::tuple<Args...>>{}>(
      [&](auto... Is) { return m.pack(std::get<Is>(t)...); });
}

// Specialization for empty tuples.  No need to pack if no arguments
constexpr bool pack_tuple_into_msg(std::tuple<>& t, dbus::message& m) {
  return true;
}

// Specialization for single types.  Used when callbacks simply return one value
template <typename Element>
constexpr bool pack_tuple_into_msg(Element& t, dbus::message& m) {
  return m.pack(t);
}

#include <dbus/impl/message_iterator.ipp>

#endif  // DBUS_MESSAGE_HPP
