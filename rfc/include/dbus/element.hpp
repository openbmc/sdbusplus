// Copyright (c) Benjamin Kietzman (github.com/bkietz)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef DBUS_ELEMENT_HPP
#define DBUS_ELEMENT_HPP

#include <dbus/dbus.h>
#include <string>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/variant.hpp>

namespace dbus {

/// Message elements
/**
 * D-Bus Messages are composed of simple elements of one of these types
 */
// bool // is this simply valid? It might pack wrong...
// http://maemo.org/api_refs/5.0/5.0-final/dbus/api/group__DBusTypes.html
typedef boost::uint8_t byte;

typedef boost::int16_t int16;
typedef boost::uint16_t uint16;
typedef boost::int32_t int32;
typedef boost::uint32_t uint32;

typedef boost::int64_t int64;
typedef boost::uint64_t uint64;
// double
// unix_fd

typedef std::string string;

typedef boost::variant<std::string, bool, byte, int16, uint16, int32, uint32,
                       int64, uint64, double>
    dbus_variant;

struct object_path {
  string value;
};
struct signature {
  string value;
};

/**
 * D-Bus Message elements are identified by unique integer type codes.
 */

template <typename InvalidType>
struct element {
  static const int code = DBUS_TYPE_INVALID;
};

template <>
struct element<bool> {
  static const int code = DBUS_TYPE_BOOLEAN;
};

template <>
struct element<byte> {
  static const int code = DBUS_TYPE_BYTE;
};

template <>
struct element<int16> {
  static const int code = DBUS_TYPE_INT16;
};

template <>
struct element<uint16> {
  static const int code = DBUS_TYPE_UINT16;
};

template <>
struct element<int32> {
  static const int code = DBUS_TYPE_INT32;
};

template <>
struct element<uint32> {
  static const int code = DBUS_TYPE_UINT32;
};

template <>
struct element<int64> {
  static const int code = DBUS_TYPE_INT64;
};

template <>
struct element<uint64> {
  static const int code = DBUS_TYPE_UINT64;
};

template <>
struct element<double> {
  static const int code = DBUS_TYPE_DOUBLE;
};

template <>
struct element<string> {
  static const int code = DBUS_TYPE_STRING;
};

template <>
struct element<dbus_variant> {
  static const int code = DBUS_TYPE_VARIANT;
};

template <>
struct element<object_path> {
  static const int code = DBUS_TYPE_OBJECT_PATH;
};

template <>
struct element<signature> {
  static const int code = DBUS_TYPE_SIGNATURE;
};

template <typename Element>
struct element<std::vector<Element>> {
  static const int code = DBUS_TYPE_ARRAY;
};

template <typename InvalidType>
struct is_fixed_type {
  static const int value = false;
};

template <>
struct is_fixed_type<bool> {
  static const int value = true;
};

template <>
struct is_fixed_type<byte> {
  static const int value = true;
};

template <>
struct is_fixed_type<int16> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint16> {
  static const int value = true;
};

template <>
struct is_fixed_type<int32> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint32> {
  static const int value = true;
};

template <>
struct is_fixed_type<int64> {
  static const int value = true;
};

template <>
struct is_fixed_type<uint64> {
  static const int value = true;
};

template <>
struct is_fixed_type<double> {
  static const int value = true;
};

template <typename InvalidType>
struct is_string_type {
  static const bool value = false;
};

template <>
struct is_string_type<string> {
  static const bool value = true;
};

template <>
struct is_string_type<object_path> {
  static const bool value = true;
};

template <>
struct is_string_type<signature> {
  static const bool value = true;
};

template <std::size_t... Is>
struct seq {};

template <std::size_t N, std::size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

template <std::size_t... Is>
struct gen_seq<0, Is...> : seq<Is...> {};

template <std::size_t N1, std::size_t... I1, std::size_t N2, std::size_t... I2>
constexpr std::array<char, N1 + N2 - 1> concat_helper(
    const std::array<char, N1>& a1, const std::array<char, N2>& a2, seq<I1...>,
    seq<I2...>) {
  return {{a1[I1]..., a2[I2]...}};
}

template <std::size_t N1, std::size_t N2>
// Initializer for the recursion
constexpr const std::array<char, N1 + N2 - 1> concat(
    const std::array<char, N1>& a1, const std::array<char, N2>& a2) {
  // note, this function expects both character arrays to be null
  // terminated. The -1 below drops the null terminator on the first string
  return concat_helper(a1, a2, gen_seq<N1 - 1>{}, gen_seq<N2>{});
}

// Base case for types that should never be asserted
template <typename Element, class Enable = void>
struct element_signature {};

// Element signature for building raw c_strings of known element_codes

// Case for fixed "final" types (double, float ect) that have their own code by
// default.,  This includes strings and variants.
// Put another way, this is the catch for everything that is not a container
template <typename Element>
struct element_signature<
    Element,
    typename std::enable_if<
        is_fixed_type<typename std::remove_pointer<Element>::type>::value ||
        is_string_type<typename std::remove_pointer<Element>::type>::value ||
        std::is_same<typename std::remove_pointer<Element>::type,
                     dbus_variant>::value>::type> {
  static auto constexpr code = std::array<char, 2>{
      {element<typename std::remove_pointer<Element>::type>::code, 0}};
};

template <typename T>
struct has_const_iterator {
 private:
  typedef char yes;
  typedef struct { char array[2]; } no;

  template <typename C>
  static yes test(typename C::const_iterator*);
  template <typename C>
  static no test(...);

 public:
  static const bool value = sizeof(test<T>(0)) == sizeof(yes);
  typedef T type;
};

// Specialization for "container" types.  Containers are defined as anything
// that can be iterated through.  This allows running any iterable type, so long
// as its value_type is a known dbus type (which could also be a container)
// Note: technically std::string is an iterable container, so it needs to be
// explicitly excluded from this specialization
template <typename Container>
struct element_signature<
    Container,
    typename std::enable_if<has_const_iterator<Container>::value &&
                            !is_string_type<Container>::value>::type> {
  static auto const constexpr code =
      concat(std::array<char, 2>{{DBUS_TYPE_ARRAY, 0}},
             element_signature<typename Container::value_type>::code);
};

// Specialization for std::pair type.  Std::pair is treated as a "dict entry"
// element.  In dbus, dictionarys are represented as arrays of dict entries, so
// this specialization builds type codes based on anything that produces a
// std::pair and constructs the signature for the dict entry, for example {si}
// would be a string of ints
template <typename Key, typename Value>
struct element_signature<std::pair<Key, Value>> {
  static auto const constexpr code =
      concat(std::array<char, 2>{{'{', 0}},
             concat(element_signature<Key>::code,
                    concat(element_signature<Value>::code,
                           std::array<char, 2>{{'}', 0}})));
};

// TODO(ed) THis can be rolled into the above definition, or genericized for all
// pointer types.  bonus, specialize for all "pointer like" types
template <typename Key, typename Value>
struct element_signature<std::pair<Key, Value>*> {
  static auto const constexpr code =
      concat(std::array<char, 2>{{'{', 0}},
             concat(element_signature<Key>::code,
                    concat(element_signature<Value>::code,
                           std::array<char, 2>{{'}', 0}})));
};

}  // namespace dbus

#endif  // DBUS_ELEMENT_HPP
