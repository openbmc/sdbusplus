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

/** @brief Read data from an sdbus message.
 *
 *  (This is an empty no-op function that is useful in some cases for
 *   variadic template reasons.)
 */
inline void read(sd_bus_message *m){};
/** @brief Read data from an sdbus message.
 *
 *  @param[in] msg - The message to read from.
 *  @tparam Args - C++ types of arguments to read from message.
 *  @param[out] args - References to place contents read from message.
 *
 *  This function will, at compile-time, deduce the DBus types of the passed
 *  C++ values and call the sd_bus_message_read functions with the
 *  appropriate type parameters.  It may also do conversions, where needed,
 *  to convert C++ types into C representations (eg. string, vector).
 */
template <typename... Args> void read(sd_bus_message *m, Args &&... args);

namespace details
{

/** @struct can_read_multiple
 *  @brief Utility to identify C++ types that may not be grouped into a
 *         single sd_bus_message_read call and instead need special
 *         handling.
 *
 *  @tparam T - Type for identification.
 *
 *  User-defined types are expected to inherit from std::false_type.
 *
 */
template <typename T> struct can_read_multiple : std::true_type
{
};
// std::string needs a char* conversion.
template <> struct can_read_multiple<std::string> : std::false_type
{
};
// object_path needs a char* conversion.
template <> struct can_read_multiple<object_path> : std::false_type
{
};
// signature needs a char* conversion.
template <> struct can_read_multiple<signature> : std::false_type
{
};
// bool needs to be resized to int, per sdbus documentation.
template <> struct can_read_multiple<bool> : std::false_type
{
};
// std::vector needs a loop.
template <typename T> struct can_read_multiple<std::vector<T>> : std::false_type
{
};
// std::pair needs to be broken down into components.
template <typename T1, typename T2>
struct can_read_multiple<std::pair<T1, T2>> : std::false_type
{
};
// std::map needs a loop.
template <typename T1, typename T2>
struct can_read_multiple<std::map<T1, T2>> : std::false_type
{
};
// std::tuple needs to be broken down into components.
template <typename... Args>
struct can_read_multiple<std::tuple<Args...>> : std::false_type
{
};
// variant needs to be broken down into components.
template <typename... Args>
struct can_read_multiple<variant<Args...>> : std::false_type
{
};

/** @struct read_single
 *  @brief Utility to read a single C++ element from a sd_bus_message.
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam S - Type of element to read.
 */
template <typename S> struct read_single
{
    // Downcast
    template <typename T> using Td = types::details::type_id_downcast_t<T>;

    /** @brief Do the operation to read element.
     *
     *  @tparam T - Type of element to read.
     *
     *  Template parameters T (function) and S (class) are different
     *  to allow the function to be utilized for many varients of S:
     *  S&, S&&, const S&, volatile S&, etc. The type_id_downcast is used
     *  to ensure T and S are equivalent.  For 'char*', this also allows
     *  use for 'char[N]' types.
     *
     *  @param[in] m - sd_bus_message to read from.
     *  @param[out] t - The reference to read item into.
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
        sd_bus_message_read_basic(m, dbusType, &t);
    }
};

template <typename T>
using read_single_t = read_single<types::details::type_id_downcast_t<T>>;

/** @brief Specialization of read_single for std::strings. */
template <> struct read_single<std::string>
{
    template <typename T> static void op(sd_bus_message *m, T &&s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        const char *str = nullptr;
        sd_bus_message_read_basic(m, dbusType, &str);
        s = str;
    }
};

/** @brief Specialization of read_single for details::string_wrapper. */
template <typename T> struct read_single<details::string_wrapper<T>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<S>());
        const char *str = nullptr;
        sd_bus_message_read_basic(m, dbusType, &str);
        s.str = str;
    }
};

/** @brief Specialization of read_single for bools. */
template <> struct read_single<bool>
{
    template <typename T> static void op(sd_bus_message *m, T &&b)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        int i = 0;
        sd_bus_message_read_basic(m, dbusType, &i);
        b = (i != 0);
    }
};

/** @brief Specialization of read_single for std::vectors. */
template <typename T> struct read_single<std::vector<T>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        s.clear();

        constexpr auto dbusType = utility::tuple_to_array(types::type_id<T>());
        sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, dbusType.data());

        while (!sd_bus_message_at_end(m, false))
        {
            std::remove_const_t<T> t{};
            sdbusplus::message::read(m, t);
            s.push_back(std::move(t));
        }

        sd_bus_message_exit_container(m);
    }
};

/** @brief Specialization of read_single for std::pairs. */
template <typename T1, typename T2> struct read_single<std::pair<T1, T2>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(
            std::tuple_cat(types::type_id_nonull<T1>(), types::type_id<T2>()));

        sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                       dbusType.data());
        sdbusplus::message::read(m, s.first, s.second);
        sd_bus_message_exit_container(m);
    }
};

/** @brief Specialization of read_single for std::maps. */
template <typename T1, typename T2> struct read_single<std::map<T1, T2>>
{
    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        s.clear();

        constexpr auto dbusType = utility::tuple_to_array(
            types::type_id<typename std::map<T1, T2>::value_type>());

        sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, dbusType.data());

        while (!sd_bus_message_at_end(m, false))
        {
            std::pair<std::remove_const_t<T1>, std::remove_const_t<T2>> p{};
            sdbusplus::message::read(m, p);
            s.insert(std::move(p));
        }

        sd_bus_message_exit_container(m);
    }
};

/** @brief Specialization of read_single for std::tuples. */
template <typename... Args> struct read_single<std::tuple<Args...>>
{
    template <typename S, std::size_t... I>
    static void _op(sd_bus_message *m, S &&s,
                    std::integer_sequence<std::size_t, I...>)
    {
        sdbusplus::message::read(m, std::get<I>(s)...);
    }

    template <typename S> static void op(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(std::tuple_cat(
            types::type_id_nonull<Args...>(),
            std::make_tuple('\0') /* null terminator for C-string */));

        sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT, dbusType.data());
        _op(m, std::forward<S>(s), std::make_index_sequence<sizeof...(Args)>());
        sd_bus_message_exit_container(m);
    }
};

/** @brief Specialization of read_single for std::variant. */
template <typename... Args> struct read_single<variant<Args...>>
{
    template <typename S, typename S1, typename... Args1>
    static void read(sd_bus_message *m, S &&s)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<S1>());

        auto rc =
            sd_bus_message_verify_type(m, SD_BUS_TYPE_VARIANT, dbusType.data());
        if (0 >= rc)
        {
            read<S, Args1...>(m, s);
            return;
        }

        std::remove_reference_t<S1> s1;

        sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, dbusType.data());
        sdbusplus::message::read(m, s1);
        sd_bus_message_exit_container(m);

        s = std::move(s1);
    }

    template <typename S> static void read(sd_bus_message *m, S &&s)
    {
        sd_bus_message_skip(m, "v");
        s = std::remove_reference_t<S>{};
    }

    template <typename S, typename = std::enable_if_t<0 < sizeof...(Args)>>
    static void op(sd_bus_message *m, S &&s)
    {
        read<S, Args...>(m, s);
    }
};

/** @brief Read a tuple of content from the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to read.
 *  @param[out] t - The tuple references to read.
 *  @tparam I - The indexes of the tuple type Tuple.
 *  @param[in] [unamed] - unused index_sequence for type deduction of I.
 */
template <typename Tuple, size_t... I>
void read_tuple(sd_bus_message *m, Tuple &&t, std::index_sequence<I...>)
{
    auto dbusTypes =
        utility::tuple_to_array(types::type_id<decltype(std::get<I>(t))...>());

    sd_bus_message_read(m, dbusTypes.data(), &std::get<I>(t)...);
}

/** @brief Read a tuple of 2 or more entries from the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to read.
 *  @param[out] t - The tuple to read into.
 *
 *  A tuple of 2 or more entries can be read as a set with
 *  sd_bus_message_read.
 */
template <typename Tuple>
std::enable_if_t<2 <= std::tuple_size<Tuple>::value>
read_tuple(sd_bus_message *m, Tuple &&t)
{
    read_tuple(m, std::move(t),
               std::make_index_sequence<std::tuple_size<Tuple>::value>());
}

/** @brief Read a tuple of exactly 1 entry from the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to read.
 *  @param[out] t - The tuple to read into.
 *
 *  A tuple of 1 entry can be read with sd_bus_message_read_basic.
 *
 *  Note: Some 1-entry tuples may need special handling due to
 *  can_read_multiple::value == false.
 */
template <typename Tuple>
std::enable_if_t<1 == std::tuple_size<Tuple>::value>
read_tuple(sd_bus_message *m, Tuple &&t)
{
    using itemType = decltype(std::get<0>(t));
    read_single_t<itemType>::op(m, std::forward<itemType>(std::get<0>(t)));
}

/** @brief Read a tuple of 0 entries - no-op.
 *
 *  This a no-op function that is useful due to variadic templates.
 */
template <typename Tuple>
std::enable_if_t<0 == std::tuple_size<Tuple>::value> inline read_tuple(
    sd_bus_message *m, Tuple &&t)
{
}

/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_read_multiple<Arg> is true.
 */
template <typename Tuple, typename Arg>
std::enable_if_t<
    can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_read_multiple<Arg> is false.
 */
template <typename Tuple, typename Arg>
std::enable_if_t<
    !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_read_multiple<Arg> is true.
 */
template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_read_multiple<Arg> is false.
 */
template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest);

template <typename Tuple, typename Arg>
std::enable_if_t<
    can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg)
{
    // Last element of a sequence and can_read_multiple, so add it to
    // the tuple and call read_tuple.

    read_tuple(m,
               std::tuple_cat(std::forward<Tuple>(t),
                              std::forward_as_tuple(std::forward<Arg>(arg))));
}

template <typename Tuple, typename Arg>
std::enable_if_t<
    !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg)
{
    // Last element of a sequence but !can_read_multiple, so call
    // read_tuple on the previous elements and separately this single
    // element.

    read_tuple(m, std::forward<Tuple>(t));
    read_tuple(m, std::forward_as_tuple(std::forward<Arg>(arg)));
}

template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest)
{
    // Not the last element of a sequence and can_read_multiple, so add it
    // to the tuple and keep grouping.

    read_grouping(m,
                  std::tuple_cat(std::forward<Tuple>(t),
                                 std::forward_as_tuple(std::forward<Arg>(arg))),
                  std::forward<Rest>(rest)...);
}

template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<
    !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message *m, Tuple &&t, Arg &&arg, Rest &&... rest)
{
    // Not the last element of a sequence but !can_read_multiple, so call
    // read_tuple on the previous elements and separately this single
    // element and then group the remaining elements.

    read_tuple(m, std::forward<Tuple>(t));
    read_tuple(m, std::forward_as_tuple(std::forward<Arg>(arg)));
    read_grouping(m, std::make_tuple(), std::forward<Rest>(rest)...);
}

} // namespace details

template <typename... Args> void read(sd_bus_message *m, Args &&... args)
{
    details::read_grouping(m, std::make_tuple(), std::forward<Args>(args)...);
}

} // namespace message

} // namespace sdbusplus
