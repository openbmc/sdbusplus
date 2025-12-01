#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/message/types.hpp>
#include <sdbusplus/sdbus.hpp>
#include <sdbusplus/utility/container_traits.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <bit>
#include <iterator>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

namespace sdbusplus
{

namespace message
{

/** @brief Append data into an sdbus message.
 *
 *  (This is an empty no-op function that is useful in some cases for
 *   variadic template reasons.)
 */
inline void append(sdbusplus::SdBusInterface* /*intf*/, sd_bus_message* /*m*/)
{}
/** @brief Append data into an sdbus message.
 *
 *  @param[in] m - The message to append to.
 *  @tparam Args - C++ types of arguments to append to message.
 *  @param[in] args - values to append to message.
 *
 *  This function will, at compile-time, deduce the DBus types of the passed
 *  C++ values and call the sd_bus_message_append functions with the
 *  appropriate type parameters.  It may also do conversions, where needed,
 *  to convert C++ types into C representations (eg. string, vector).
 */
template <typename... Args>
void append(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Args&&... args);

namespace details
{

/** @brief Utility to append a single C++ element into a sd_bus_message.
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam S - Type of element to append.
 */
template <typename S, typename Enable = void>
struct append_single
{
    // Downcast
    template <typename T>
    using Td = types::details::type_id_downcast_t<T>;

    // sd_bus_message_append_basic expects a T* (cast to void*) for most types,
    // so t& is appropriate.  In the case of char*, it expects the void* is
    // the char*.  If we use &t, that is a char** and not a char*.
    //
    // Use these helper templates 'address_of(t)' in place of '&t' to
    // handle both cases.
    template <typename T>
    static auto address_of_helper(T&& t, std::false_type)
    {
        return &t;
    }
    template <typename T>
    static auto address_of_helper(T&& t, std::true_type)
    {
        return t;
    }

    template <typename T>
    static auto address_of(T&& t)
    {
        return address_of_helper(std::forward<T>(t),
                                 std::is_pointer<std::remove_reference_t<T>>());
    }

    /** @brief Do the operation to append element.
     *
     *  @tparam T - Type of element to append.
     *
     *  Template parameters T (function) and S (class) are different
     *  to allow the function to be utilized for many variants of S:
     *  S&, S&&, const S&, volatile S&, etc. The type_id_downcast is used
     *  to ensure T and S are equivalent.  For 'char*', this also allows
     *  use for 'char[N]' types.
     *
     *  @param[in] m - sd_bus_message to append into.
     *  @param[in] t - The item to append.
     */
    template <typename T>
    static std::enable_if_t<std::is_same_v<S, Td<T>> && !std::is_enum_v<Td<T>>>
        op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
    {
        // For this default implementation, we need to ensure that only
        // basic types are used.
        static_assert(std::is_fundamental_v<Td<T>> ||
                          std::is_convertible_v<Td<T>, const char*>,
                      "Non-basic types are not allowed.");

        static constexpr auto dbusType = types::type_id<T>()[0];
        intf->sd_bus_message_append_basic(m, dbusType,
                                          address_of(std::forward<T>(t)));
    }

    template <typename T>
    static std::enable_if_t<std::is_same_v<S, Td<T>> && std::is_enum_v<Td<T>>>
        op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
    {
        auto value = sdbusplus::message::convert_to_string<Td<T>>(t);
        sdbusplus::message::append(intf, m, value);
    }
};

template <typename T>
using append_single_t = append_single<types::details::type_id_downcast_t<T>>;

/** @brief Specialization of append_single for details::unix_fd. */
template <>
struct append_single<details::unix_fd_type>
{
    template <typename T>
    static void sanitize(const T&)
    {}

    template <typename T>
    static void sanitize(T& s)
    {
        s.fd = -1;
    }

    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& s)
    {
        static constexpr auto dbusType = types::type_id<T>()[0];
        intf->sd_bus_message_append_basic(m, dbusType, &s.fd);

        // sd-bus now owns the file descriptor
        sanitize(s);
    }
};

/** @brief Specialization of append_single for std::strings. */
template <>
struct append_single<std::string>
{
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& s)
    {
        static constexpr auto dbusType = types::type_id<T>()[0];
        intf->sd_bus_message_append_basic(m, dbusType, s.c_str());
    }
};

/** @brief Specialization of append_single for std::string_views. */
template <>
struct append_single<std::string_view>
{
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& s)
    {
        iovec iov{std::bit_cast<void*>(s.data()), s.size()};
        intf->sd_bus_message_append_string_iovec(m, &iov, 1);
    }
};

/** @brief Specialization of append_single for details::string_wrapper. */
template <>
struct append_single<details::string_wrapper>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        static constexpr auto dbusType = types::type_id<S>()[0];
        intf->sd_bus_message_append_basic(m, dbusType, s.str.c_str());
    }
};

/** @brief Specialization of append_single for details::string_wrapper. */
template <>
struct append_single<details::string_path_wrapper>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        static constexpr auto dbusType = types::type_id<S>()[0];
        intf->sd_bus_message_append_basic(m, dbusType, s.str.c_str());
    }
};

/** @brief Specialization of append_single for bool. */
template <>
struct append_single<bool>
{
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& b)
    {
        static constexpr auto dbusType = types::type_id<T>()[0];
        int i = b;
        intf->sd_bus_message_append_basic(m, dbusType, &i);
    }
};

// Determines if fallback to normal iteration and append is required (can't use
// sd_bus_message_append_array)
template <typename T>
concept can_append_array_non_contigious =
    utility::is_dbus_array<T> && !utility::can_append_array_value<T>;

/** @brief Specialization of append_single for containers
 * (ie vector, array, etc), where the contiguous elements can be loaded in a
 * single operation
 */
template <can_append_array_non_contigious T>
struct append_single<T>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        static const auto dbusType = types::type_id<typename T::value_type>();

        intf->sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY,
                                            dbusType.c_str());
        for (const typename T::value_type& i : s)
        {
            sdbusplus::message::append(intf, m, i);
        }
        intf->sd_bus_message_close_container(m);
    }
};

// Determines if the iterable type (vector, array) meets the requirements for
// using sd_bus_message_append_array
template <typename T>
concept can_append_array_contigious =
    utility::is_dbus_array<T> && utility::can_append_array_value<T>;

/** @brief Specialization of append_single for vector and array T,
 * with its elements is trivially copyable, and is an integral type,
 * Bool is explicitly disallowed by sd-bus, so avoid it here */
template <can_append_array_contigious T>
struct append_single<T>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        static constexpr auto dbusType = types::type_id<T>()[1];
        intf->sd_bus_message_append_array(
            m, dbusType, s.data(), s.size() * sizeof(typename T::value_type));
    }
};

/** @brief Specialization of append_single for std::pairs. */
template <typename T1, typename T2>
struct append_single<std::pair<T1, T2>>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        static const auto dbusType = types::type_id<T1, T2>();

        intf->sd_bus_message_open_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                            dbusType.c_str());
        sdbusplus::message::append(intf, m, s.first, s.second);
        intf->sd_bus_message_close_container(m);
    }
};

/** @brief Specialization of append_single for std::tuples. */
template <typename... Args>
struct append_single<std::tuple<Args...>>
{
    template <typename S, std::size_t... I>
    static void _op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s,
                    std::integer_sequence<std::size_t, I...>)
    {
        sdbusplus::message::append(intf, m, std::get<I>(s)...);
    }

    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        static const auto dbusType = types::type_id<Args...>();

        intf->sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT,
                                            dbusType.c_str());
        _op(intf, m, std::forward<S>(s),
            std::make_index_sequence<sizeof...(Args)>());
        intf->sd_bus_message_close_container(m);
    }
};

/** @brief Specialization of append_single for std::variant. */
template <typename... Args>
struct append_single<std::variant<Args...>>
{
    template <typename S, typename = std::enable_if_t<0 < sizeof...(Args)>>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        auto apply = [intf, m](auto&& arg) {
            static const auto dbusType = types::type_id<decltype(arg)>();

            intf->sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT,
                                                dbusType.c_str());
            sdbusplus::message::append(intf, m, arg);
            intf->sd_bus_message_close_container(m);
        };

        std::visit(apply, s);
    }
};
} // namespace details

template <typename... Args>
void append(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Args&&... args)
{
    (details::append_single_t<Args>::op(intf, m, args), ...);
}

} // namespace message

} // namespace sdbusplus
