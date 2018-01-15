#pragma once

#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <sdbusplus/utility/type_traits.hpp>
#include <systemd/sd-bus.h>
#include <tuple>

namespace sdbusplus
{

namespace message
{

/** @brief Append data into an sdbus message.
 *
 *  (This is an empty no-op function that is useful in some cases for
 *   variadic template reasons.)
 */
inline void append(sd_bus_message *m){};
/** @brief Append data into an sdbus message.
 *
 *  @param[in] msg - The message to append to.
 *  @tparam Args - C++ types of arguments to append to message.
 *  @param[in] args - values to append to message.
 *
 *  This function will, at compile-time, deduce the DBus types of the passed
 *  C++ values and call the sd_bus_message_append functions with the
 *  appropriate type parameters.  It may also do conversions, where needed,
 *  to convert C++ types into C representations (eg. string, vector).
 */
template <typename... Args> void append(sd_bus_message *m, Args &&... args);

namespace details
{

/** @struct can_append_multiple
 *  @brief Utility to identify C++ types that may not be grouped into a
 *         single sd_bus_message_append call and instead need special
 *         handling.
 *
 *  @tparam T - Type for identification.
 *
 *  User-defined types are expected to inherit from std::false_type.
 *
 */
template <typename T> struct can_append_multiple : std::true_type
{
};
// std::string needs a c_str() call.
template <> struct can_append_multiple<std::string> : std::false_type
{
};
// object_path needs a c_str() call.
template <> struct can_append_multiple<object_path> : std::false_type
{
};
// signature needs a c_str() call.
template <> struct can_append_multiple<signature> : std::false_type
{
};
// bool needs to be resized to int, per sdbus documentation.
template <> struct can_append_multiple<bool> : std::false_type
{
};
// std::vector needs a loop.
template <typename T>
struct can_append_multiple<std::vector<T>> : std::false_type
{
};
// std::pair needs to be broken down into components.
template <typename T1, typename T2>
struct can_append_multiple<std::pair<T1, T2>> : std::false_type
{
};
// std::map needs a loop.
template <typename T1, typename T2>
struct can_append_multiple<std::map<T1, T2>> : std::false_type
{
};
// std::tuple needs to be broken down into components.
template <typename... Args>
struct can_append_multiple<std::tuple<Args...>> : std::false_type
{
};
// variant needs to be broken down into components.
template <typename... Args>
struct can_append_multiple<variant<Args...>> : std::false_type
{
};

/** @struct append_single
 *  @brief Utility to append a single C++ element into a sd_bus_message.
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam S - Type of element to append.
 */
template <typename S> struct append_single
{
    // Downcast
    template <typename T> using Td = types::details::type_id_downcast_t<T>;

    // sd_bus_message_append_basic expects a T* (cast to void*) for most types,
    // so t& is appropriate.  In the case of char*, it expects the void* is
    // the char*.  If we use &t, that is a char** and not a char*.
    //
    // Use these helper templates 'address_of(t)' in place of '&t' to
    // handle both cases.
    template <typename T> static auto address_of_helper(T &&t, std::false_type)
    {
        return &t;
    }
    template <typename T> static auto address_of_helper(T &&t, std::true_type)
    {
        return t;
    }

    template <typename T> static auto address_of(T &&t)
    {
        return address_of_helper(std::forward<T>(t),
                                 std::is_pointer<std::remove_reference_t<T>>());
    }

    /** @brief Do the operation to append element.
     *
     *  @tparam T - Type of element to append.
     *
     *  Template parameters T (function) and S (class) are different
     *  to allow the function to be utilized for many varients of S:
     *  S&, S&&, const S&, volatile S&, etc. The type_id_downcast is used
     *  to ensure T and S are equivalent.  For 'char*', this also allows
     *  use for 'char[N]' types.
     *
     *  @param[in] m - sd_bus_message to append into.
     *  @param[in] t - The item to append.
     */
    template <typename T,
              typename = std::enable_if_t<std::is_same<S, Td<T>>::value>>
    static void op(sd_bus_message *m, T &&t)
    {
        // For this default implementation, we need to ensure that only
        // basic types are used.
        static_assert(std::is_fundamental<Td<T>>::value ||
                          std::is_convertible<Td<T>, const char *>::value,
                      "Non-basic types are not allowed.");

        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        sd_bus_message_append_basic(m, dbusType,
                                    address_of(std::forward<T>(t)));
    }
};

template <typename T>
using append_single_t = append_single<types::details::type_id_downcast_t<T>>;

/** @brief Specialization of append_single for std::strings. */
template <> struct append_single<std::string>
{
    template <typename T> static void op(sd_bus_message *m, T &&s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        sd_bus_message_append_basic(m, dbusType, s.c_str());
    }
};

/** @brief Specialization of append_single for details::string_wrapper. */
template <typename T> struct append_single<details::string_wrapper<T>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<S>());
        sd_bus_message_append_basic(m, dbusType, s.str.c_str());
    }
};

/** @brief Specialization of append_single for bool. */
template <> struct append_single<bool>
{
    template <typename T> static void op(sd_bus_message *m, T &&b)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        int i = b;
        sd_bus_message_append_basic(m, dbusType, &i);
    }
};

/** @brief Specialization of append_single for std::vectors. */
template <typename T> struct append_single<std::vector<T>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<T>());

        sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, dbusType.data());
        for (auto &i : s)
        {
            sdbusplus::message::append(m, i);
        }
        sd_bus_message_close_container(m);
    }
};

/** @brief Specialization of append_single for std::pairs. */
template <typename T1, typename T2> struct append_single<std::pair<T1, T2>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(
            std::tuple_cat(types::type_id_nonull<T1>(), types::type_id<T2>()));

        sd_bus_message_open_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                      dbusType.data());
        sdbusplus::message::append(m, s.first, s.second);
        sd_bus_message_close_container(m);
    }
};

/** @brief Specialization of append_single for std::maps. */
template <typename T1, typename T2> struct append_single<std::map<T1, T2>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(
            types::type_id<typename std::map<T1, T2>::value_type>());

        sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY, dbusType.data());
        for (auto &i : s)
        {
            sdbusplus::message::append(m, i);
        }
        sd_bus_message_close_container(m);
    }
};

/** @brief Specialization of append_single for std::tuples. */
template <typename... Args> struct append_single<std::tuple<Args...>>
{
    template <typename S, std::size_t... I>
    static void _op(sd_bus_message *m, S &&s,
                    std::integer_sequence<std::size_t, I...>)
    {
        sdbusplus::message::append(m, std::get<I>(s)...);
    }

    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(std::tuple_cat(
            types::type_id_nonull<Args...>(),
            std::make_tuple('\0') /* null terminator for C-string */));

        sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT, dbusType.data());
        _op(m, std::forward<S>(s), std::make_index_sequence<sizeof...(Args)>());
        sd_bus_message_close_container(m);
    }
};

/** @brief Specialization of append_single for std::variant. */
template <typename... Args> struct append_single<variant<Args...>>
{
    template <typename S, typename = std::enable_if_t<0 < sizeof...(Args)>>
    static void op(sd_bus_message *m, S &&s)
    {
        auto apply = [m](auto &&arg) {
            constexpr auto dbusType =
                utility::tuple_to_array(types::type_id<decltype(arg)>());

            sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT,
                                          dbusType.data());
            sdbusplus::message::append(m, arg);
            sd_bus_message_close_container(m);
        };

        std::remove_reference_t<S>::visit(s, apply);
    }
};

/** @brief Append a tuple of content into the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to append.
 *  @param[in] t - The tuple value to append.
 *  @tparam I - The indexes of the tuple type Tuple.
 *  @param[in] [unamed] - unused index_sequence for type deduction of I.
 */
template <typename Tuple, size_t... I>
void append_tuple(sd_bus_message *m, Tuple &&t, std::index_sequence<I...>)
{
    auto dbusTypes =
        utility::tuple_to_array(types::type_id<decltype(std::get<I>(t))...>());

    sd_bus_message_append(m, dbusTypes.data(), std::get<I>(t)...);
}

/** @brief Append a tuple of 2 or more entries into the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to append.
 *  @param[in] t - The tuple to append.
 *
 *  A tuple of 2 or more entries can be added as a set with
 *  sd_bus_message_append.
 */
template <typename Tuple>
std::enable_if_t<2 <= std::tuple_size<Tuple>::value>
append_tuple(sd_bus_message *m, Tuple &&t)
{
    append_tuple(m, std::move(t),
                 std::make_index_sequence<std::tuple_size<Tuple>::value>());
}

/** @brief Append a tuple of exactly 1 entry into the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to append.
 *  @param[in] t - The tuple to append.
 *
 *  A tuple of 1 entry can be added with sd_bus_message_append_basic.
 *
 *  Note: Some 1-entry tuples may need special handling due to
 *  can_append_multiple::value == false.
 */
template <typename Tuple>
std::enable_if_t<1 == std::tuple_size<Tuple>::value>
append_tuple(sd_bus_message *m, Tuple &&t)
{
    using itemType = decltype(std::get<0>(t));
    append_single_t<itemType>::op(m, std::forward<itemType>(std::get<0>(t)));
}

/** @brief Append a tuple of 0 entries - no-op.
 *
 *  This a no-op function that is useful due to variadic templates.
 */
template <typename Tuple>
std::enable_if_t<0 == std::tuple_size<Tuple>::value> inline append_tuple(
    sd_bus_message *m, Tuple &&t)
{
}

/** @brief Group a sequence of C++ types for appending into an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_append_multiple<Arg> is true.
 */
template <typename Tuple, typename Arg>
std::enable_if_t<
    can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg);
/** @brief Group a sequence of C++ types for appending into an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_append_multiple<Arg> is false.
 */
template <typename Tuple, typename Arg>
std::enable_if_t<
    !can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg);
/** @brief Group a sequence of C++ types for appending into an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_append_multiple<Arg> is true.
 */
template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest);
/** @brief Group a sequence of C++ types for appending into an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_append_multiple<Arg> is false.
 */
template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    !can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest);

template <typename Tuple, typename Arg>
std::enable_if_t<
    can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg)
{
    // Last element of a sequence and can_append_multiple, so add it to
    // the tuple and call append_tuple.

    append_tuple(m,
                 std::tuple_cat(std::forward<Tuple>(t),
                                std::forward_as_tuple(std::forward<Arg>(arg))));
}

template <typename Tuple, typename Arg>
std::enable_if_t<
    !can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg)
{
    // Last element of a sequence but !can_append_multiple, so call
    // append_tuple on the previous elements and separately this single
    // element.

    append_tuple(m, std::forward<Tuple>(t));
    append_tuple(m, std::forward_as_tuple(std::forward<Arg>(arg)));
}

template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest)
{
    // Not the last element of a sequence and can_append_multiple, so add it
    // to the tuple and keep grouping.

    append_grouping(
        m,
        std::tuple_cat(std::forward<Tuple>(t),
                       std::forward_as_tuple(std::forward<Arg>(arg))),
        std::forward<Rest>(rest)...);
}

template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    !can_append_multiple<types::details::type_id_downcast_t<Arg>>::value>
append_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest)
{
    // Not the last element of a sequence but !can_append_multiple, so call
    // append_tuple on the previous elements and separately this single
    // element and then group the remaining elements.

    append_tuple(m, std::forward<Tuple>(t));
    append_tuple(m, std::forward_as_tuple(std::forward<Arg>(arg)));
    append_grouping(m, std::make_tuple(), std::forward<Rest>(rest)...);
}

} // namespace details

template <typename... Args> void append(sd_bus_message *m, Args &&... args)
{
    details::append_grouping(m, std::make_tuple(), std::forward<Args>(args)...);
}

} // namespace message

} // namespace sdbusplus
