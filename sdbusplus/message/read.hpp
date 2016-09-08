#pragma once

#include <tuple>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/type_traits.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <systemd/sd-bus.h>

namespace sdbusplus
{

namespace message
{

/** @brief Read data from an sdbus message.
 *
 *  (This is an empty no-op function that is useful in some cases for
 *   variadic template reasons.)
 */
inline void read(sd_bus_message* m) {};
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
template <typename ...Args> void read(sd_bus_message* m, Args&&... args);

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
template<typename T> struct can_read_multiple : std::true_type {};
    // std::string needs a c_str() call.
template<> struct can_read_multiple<std::string> : std::false_type {};

/** @struct read_single
 *  @brief Utility to read a single C++ element from a sd_bus_message.
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam S - Type of element to read.
 */
template<typename S> struct read_single
{
    // Downcast
    template<typename T>
    using Td = types::details::type_id_downcast_t<T>;

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
    template<typename T,
             typename = std::enable_if_t<std::is_same<S, Td<T>>::value>>
    static void op(sd_bus_message* m, T&& t)
    {
        // For this default implementation, we need to ensure that only
        // basic types are used.
        static_assert(std::is_fundamental<Td<T>>::value ||
                      std::is_convertible<Td<T>, const char*>::value,
                      "Non-basic types are not allowed.");

        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        sd_bus_message_read_basic(m, dbusType, &t);
    }
};

template<typename T> using read_single_t =
        read_single<types::details::type_id_downcast_t<T>>;

/** @brief Specialization of read_single for std::strings. */
template <> struct read_single<std::string>
{
    template<typename T>
    static void op(sd_bus_message* m, T&& s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        const char* str = nullptr;
        sd_bus_message_read_basic(m, dbusType, &str);
        s = str;
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
void read_tuple(sd_bus_message* m, Tuple&& t, std::index_sequence<I...>)
{
    auto dbusTypes = utility::tuple_to_array(
            types::type_id<decltype(std::get<I>(t))...>());

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
template <typename Tuple> std::enable_if_t<2 <= std::tuple_size<Tuple>::value>
read_tuple(sd_bus_message* m, Tuple&& t)
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
template <typename Tuple> std::enable_if_t<1 == std::tuple_size<Tuple>::value>
read_tuple(sd_bus_message* m, Tuple&& t)
{
    using itemType = decltype(std::get<0>(t));
    read_single_t<itemType>::op(m, std::forward<itemType>(std::get<0>(t)));
}

/** @brief Read a tuple of 0 entries - no-op.
 *
 *  This a no-op function that is useful due to variadic templates.
 */
template <typename Tuple> std::enable_if_t<0 == std::tuple_size<Tuple>::value>
inline read_tuple(sd_bus_message* m, Tuple&& t) {}

/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_read_multiple<Arg> is true.
 */
template <typename Tuple, typename Arg> std::enable_if_t<
        can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_read_multiple<Arg> is false.
 */
template <typename Tuple, typename Arg> std::enable_if_t<
        !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_read_multiple<Arg> is true.
 */
template <typename Tuple, typename Arg, typename ...Rest> std::enable_if_t<
        can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg, Rest&&... rest);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_read_multiple<Arg> is false.
 */
template <typename Tuple, typename Arg, typename ...Rest> std::enable_if_t<
        !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg, Rest&&... rest);

template <typename Tuple, typename Arg> std::enable_if_t<
        can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg)
{
    // Last element of a sequence and can_read_multiple, so add it to
    // the tuple and call read_tuple.

    read_tuple(m, std::tuple_cat(std::forward<Tuple>(t),
                                 std::forward_as_tuple(
                                     std::forward<Arg>(arg))));
}

template <typename Tuple, typename Arg> std::enable_if_t<
        !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg)
{
    // Last element of a sequence but !can_read_multiple, so call
    // read_tuple on the previous elements and separately this single
    // element.

    read_tuple(m, std::forward<Tuple>(t));
    read_tuple(m, std::forward_as_tuple(std::forward<Arg>(arg)));
}

template <typename Tuple, typename Arg, typename ...Rest> std::enable_if_t<
        can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg, Rest&&... rest)
{
    // Not the last element of a sequence and can_read_multiple, so add it
    // to the tuple and keep grouping.

    read_grouping(m, std::tuple_cat(std::forward<Tuple>(t),
                                    std::forward_as_tuple(
                                          std::forward<Arg>(arg))),
                  std::forward<Rest>(rest)...);
}

template <typename Tuple, typename Arg, typename ...Rest> std::enable_if_t<
        !can_read_multiple<types::details::type_id_downcast_t<Arg>>::value>
read_grouping(sd_bus_message* m, Tuple&& t, Arg&& arg, Rest&&... rest)
{
    // Not the last element of a sequence but !can_read_multiple, so call
    // read_tuple on the previous elements and separately this single
    // element and then group the remaining elements.

    read_tuple(m, std::forward<Tuple>(t));
    read_tuple(m, std::forward_as_tuple(std::forward<Arg>(arg)));
    read_grouping(m, std::make_tuple(), std::forward<Rest>(rest)...);
}

} // namespace details

template <typename ...Args> void read(sd_bus_message* m, Args&&... args)
{
    details::read_grouping(m, std::make_tuple(), std::forward<Args>(args)...);
}

} // namespace message

} // namespace sdbusplus
