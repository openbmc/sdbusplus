#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace sdbusplus
{

namespace message
{

/** @brief Read data from an sdbus message.
 *
 *  (This is an empty no-op function that is useful in some cases for
 *   variadic template reasons.)
 */
inline void read(sdbusplus::SdBusInterface* /*intf*/, sd_bus_message* /*m*/) {}
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
template <typename... Args>
void read(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Args&&... args);

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
 *  Enums are converted from strings, so must be done one at a time.
 *
 */
template <typename T, typename Enable = void>
struct can_read_multiple :
    std::conditional_t<std::is_enum_v<T>, std::false_type, std::true_type>
{};
// unix_fd's int needs to be wrapped
template <>
struct can_read_multiple<unix_fd> : std::false_type
{};
// std::string needs a char* conversion.
template <>
struct can_read_multiple<std::string> : std::false_type
{};
// object_path needs a char* conversion.
template <>
struct can_read_multiple<object_path> : std::false_type
{};
// signature needs a char* conversion.
template <>
struct can_read_multiple<signature> : std::false_type
{};
// bool needs to be resized to int, per sdbus documentation.
template <>
struct can_read_multiple<bool> : std::false_type
{};

// std::vector/map/unordered_vector/set need loops
template <typename T>
struct can_read_multiple<
    T, typename std::enable_if_t<utility::has_emplace_method_v<T> ||
                                 utility::has_emplace_back_method_v<T>>> :
    std::false_type
{};

// std::pair needs to be broken down into components.
template <typename T1, typename T2>
struct can_read_multiple<std::pair<T1, T2>> : std::false_type
{};

// std::tuple needs to be broken down into components.
template <typename... Args>
struct can_read_multiple<std::tuple<Args...>> : std::false_type
{};
// variant needs to be broken down into components.
template <typename... Args>
struct can_read_multiple<std::variant<Args...>> : std::false_type
{};

template <typename... Args>
inline constexpr bool can_read_multiple_v = can_read_multiple<Args...>::value;

/** @struct read_single
 *  @brief Utility to read a single C++ element from a sd_bus_message.
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam S - Type of element to read.
 */
template <typename S, typename Enable = void>
struct read_single
{
    // Downcast
    template <typename T>
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
    template <typename T>
    static std::enable_if_t<std::is_same_v<S, Td<T>> && !std::is_enum_v<Td<T>>>
        op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
    {
        // For this default implementation, we need to ensure that only
        // basic types are used.
        static_assert(std::is_fundamental_v<Td<T>> ||
                          std::is_convertible_v<Td<T>, const char*>,
                      "Non-basic types are not allowed.");

        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        int r = intf->sd_bus_message_read_basic(m, dbusType, &t);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_read_basic fundamental");
        }
    }

    template <typename T>
    static std::enable_if_t<std::is_same_v<S, Td<T>> && std::is_enum_v<Td<T>>>
        op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
    {
        std::string value{};
        sdbusplus::message::read(intf, m, value);

        auto r = sdbusplus::message::convert_from_string<Td<T>>(value);
        if (!r)
        {
            throw sdbusplus::exception::InvalidEnumString();
        }
        t = *r;
    }
};

template <typename T>
using read_single_t = read_single<types::details::type_id_downcast_t<T>>;

/** @brief Specialization of read_single for details::unix_fd. */
template <>
struct read_single<details::unix_fd_type>
{
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        int r = intf->sd_bus_message_read_basic(m, dbusType, &s.fd);
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_read_basic unix_fd");
        }
    }
};

/** @brief Specialization of read_single for various string class types.
 *
 *  Supports std::strings, details::string_wrapper and
 *  details::string_path_wrapper.
 */
template <typename T>
struct read_single<
    T, std::enable_if_t<std::is_same_v<T, std::string> ||
                        std::is_same_v<T, details::string_wrapper> ||
                        std::is_same_v<T, details::string_path_wrapper>>>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<S>());
        const char* str = nullptr;
        int r = intf->sd_bus_message_read_basic(m, dbusType, &str);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_read_basic string");
        }
        s = T(str);
    }
};

/** @brief Specialization of read_single for bools. */
template <>
struct read_single<bool>
{
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& b)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<T>());
        int i = 0;
        int r = intf->sd_bus_message_read_basic(m, dbusType, &i);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_read_basic bool");
        }
        b = (i != 0);
    }
};

/** @brief Specialization of read_single for std::vectors. */
template <typename T>
struct read_single<T, std::enable_if_t<utility::has_emplace_back_method_v<T>>>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<T>());
        int r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY,
                                                     dbusType.data() + 1);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_enter_container emplace_back_container");
        }

        while (!(r = intf->sd_bus_message_at_end(m, false)))
        {
            types::details::type_id_downcast_t<typename T::value_type> t;
            sdbusplus::message::read(intf, m, t);
            s.emplace_back(std::move(t));
        }
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_at_end emplace_back_container");
        }

        r = intf->sd_bus_message_exit_container(m);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_exit_container emplace_back_container");
        }
    }
};

/** @brief Specialization of read_single for std::map. */
template <typename T>
struct read_single<T, std::enable_if_t<utility::has_emplace_method_v<T>>>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<T>());
        int r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY,
                                                     dbusType.data() + 1);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_enter_container emplace_container");
        }

        while (!(r = intf->sd_bus_message_at_end(m, false)))
        {
            types::details::type_id_downcast_t<typename T::value_type> t;
            sdbusplus::message::read(intf, m, t);
            s.emplace(std::move(t));
        }
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_at_end emplace_container");
        }

        r = intf->sd_bus_message_exit_container(m);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_exit_container emplace_container");
        }
    }
};

/** @brief Specialization of read_single for std::pairs. */
template <typename T1, typename T2>
struct read_single<std::pair<T1, T2>>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        constexpr auto dbusType = utility::tuple_to_array(
            std::tuple_cat(types::type_id_nonull<T1>(), types::type_id<T2>()));

        int r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                                     dbusType.data());
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_enter_container pair");
        }

        sdbusplus::message::read(intf, m, s.first, s.second);

        r = intf->sd_bus_message_exit_container(m);
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_exit_container pair");
        }
    }
};

/** @brief Specialization of read_single for std::tuples. */
template <typename... Args>
struct read_single<std::tuple<Args...>>
{
    template <typename S, std::size_t... I>
    static void _op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s,
                    std::integer_sequence<std::size_t, I...>)
    {
        sdbusplus::message::read(intf, m, std::get<I>(s)...);
    }

    template <typename S>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        constexpr auto dbusType = utility::tuple_to_array(std::tuple_cat(
            types::type_id_nonull<Args...>(),
            std::make_tuple('\0') /* null terminator for C-string */));

        int r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT,
                                                     dbusType.data());
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_enter_container tuple");
        }

        _op(intf, m, std::forward<S>(s),
            std::make_index_sequence<sizeof...(Args)>());

        r = intf->sd_bus_message_exit_container(m);
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_exit_container tuple");
        }
    }
};

/** @brief Specialization of read_single for std::variant. */
template <typename... Args>
struct read_single<std::variant<Args...>>
{
    // Downcast
    template <typename T>
    using Td = types::details::type_id_downcast_t<T>;

    template <typename S, typename S1, typename... Args1>
    static void read(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<S1>());

        int r = intf->sd_bus_message_verify_type(m, SD_BUS_TYPE_VARIANT,
                                                 dbusType.data());
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_verify_type variant");
        }
        if (!r)
        {
            read<S, Args1...>(intf, m, s);
            return;
        }

        r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT,
                                                 dbusType.data());
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_enter_container variant");
        }

        // If this type is an enum or string, we don't know which is the
        // valid parsing.  Delegate to 'convert_from_string' so we do the
        // correct conversion.
        if constexpr (std::is_enum_v<Td<S1>> ||
                      std::is_same_v<std::string, Td<S1>>)
        {
            std::string str{};
            sdbusplus::message::read(intf, m, str);
            auto ret =
                sdbusplus::message::convert_from_string<std::variant<Args...>>(
                    str);

            if (!ret)
            {
                throw sdbusplus::exception::InvalidEnumString();
            }

            s = std::move(*ret);
        }
        else // otherise, read it out directly.
        {
            std::remove_reference_t<S1> s1;
            sdbusplus::message::read(intf, m, s1);
            s = std::move(s1);
        }

        r = intf->sd_bus_message_exit_container(m);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_exit_container variant");
        }
    }

    template <typename S>
    static void read(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        int r = intf->sd_bus_message_skip(m, "v");
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_skip variant");
        }
        s = std::remove_reference_t<S>{};
    }

    template <typename S, typename = std::enable_if_t<0 < sizeof...(Args)>>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S&& s)
    {
        read<S, Args...>(intf, m, s);
    }
};

/** @brief Specialization of read_single for std::monostate. */
template <>
struct read_single<std::monostate>
{
    template <typename S>
    static void op(sdbusplus::SdBusInterface*, sd_bus_message*, S&&)
    {}
};

template <typename T>
static void tuple_item_read(sdbusplus::SdBusInterface* intf, sd_bus_message* m,
                            T&& t)
{
    sdbusplus::message::read(intf, m, t);
}

template <int Index>
struct ReadHelper
{
    template <typename... Fields>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m,
                   std::tuple<Fields...> field_tuple)
    {
        auto& field = std::get<Index - 1>(field_tuple);

        ReadHelper<Index - 1>::op(intf, m, std::move(field_tuple));

        tuple_item_read(intf, m, field);
    }
};

template <>
struct ReadHelper<1>
{
    template <typename... Fields>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m,
                   std::tuple<Fields...> field_tuple)
    {
        tuple_item_read(intf, m, std::get<0>(field_tuple));
    }
};

/** @brief Read a tuple of 2 or more entries from the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to read.
 *  @param[out] t - The tuple to read into.
 *
 *  A tuple of 2 or more entries can be read as a set with
 *  sd_bus_message_read.
 */
template <typename Tuple>
std::enable_if_t<2 <= std::tuple_size_v<Tuple>>
    read_tuple(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t)
{
    ReadHelper<std::tuple_size_v<Tuple>>::op(intf, m, std::move(t));
}

/** @brief Read a tuple of exactly 1 entry from the sd_bus_message.
 *
 *  @tparam Tuple - The tuple type to read.
 *  @param[out] t - The tuple to read into.
 *
 *  A tuple of 1 entry can be read with sd_bus_message_read_basic.
 *
 *  Note: Some 1-entry tuples may need special handling due to
 *  can_read_multiple_v == false.
 */
template <typename Tuple>
std::enable_if_t<1 == std::tuple_size_v<Tuple>>
    read_tuple(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t)
{
    using itemType = decltype(std::get<0>(t));
    read_single_t<itemType>::op(intf, m,
                                std::forward<itemType>(std::get<0>(t)));
}

/** @brief Read a tuple of 0 entries - no-op.
 *
 *  This a no-op function that is useful due to variadic templates.
 */
template <typename Tuple>
std::enable_if_t<0 == std::tuple_size_v<Tuple>> inline read_tuple(
    sdbusplus::SdBusInterface* /*intf*/, sd_bus_message* /*m*/, Tuple&& /*t*/)
{}

/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_read_multiple_v<Arg> is true.
 */
template <typename Tuple, typename Arg>
std::enable_if_t<can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *
 *  Specialization for when can_read_multiple_v<Arg> is false.
 */
template <typename Tuple, typename Arg>
std::enable_if_t<!can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_read_multiple_v<Arg> is true.
 */
template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg, Rest&&... rest);
/** @brief Group a sequence of C++ types for reading from an sd_bus_message.
 *  @tparam Tuple - A tuple of previously analyzed types.
 *  @tparam Arg - The argument to analyze for grouping.
 *  @tparam Rest - The remaining arguments left to analyze.
 *
 *  Specialization for when can_read_multiple_v<Arg> is false.
 */
template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<!can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg, Rest&&... rest);

template <typename Tuple, typename Arg>
std::enable_if_t<can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg)
{
    // Last element of a sequence and can_read_multiple, so add it to
    // the tuple and call read_tuple.

    read_tuple(intf, m,
               std::tuple_cat(std::forward<Tuple>(t),
                              std::forward_as_tuple(std::forward<Arg>(arg))));
}

template <typename Tuple, typename Arg>
std::enable_if_t<!can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg)
{
    // Last element of a sequence but !can_read_multiple, so call
    // read_tuple on the previous elements and separately this single
    // element.

    read_tuple(intf, m, std::forward<Tuple>(t));
    read_tuple(intf, m, std::forward_as_tuple(std::forward<Arg>(arg)));
}

template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg, Rest&&... rest)
{
    // Not the last element of a sequence and can_read_multiple, so add it
    // to the tuple and keep grouping.

    read_grouping(intf, m,
                  std::tuple_cat(std::forward<Tuple>(t),
                                 std::forward_as_tuple(std::forward<Arg>(arg))),
                  std::forward<Rest>(rest)...);
}

template <typename Tuple, typename Arg, typename... Rest>
std::enable_if_t<!can_read_multiple_v<types::details::type_id_downcast_t<Arg>>>
    read_grouping(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Tuple&& t,
                  Arg&& arg, Rest&&... rest)
{
    // Not the last element of a sequence but !can_read_multiple, so call
    // read_tuple on the previous elements and separately this single
    // element and then group the remaining elements.

    read_tuple(intf, m, std::forward<Tuple>(t));
    read_tuple(intf, m, std::forward_as_tuple(std::forward<Arg>(arg)));
    read_grouping(intf, m, std::make_tuple(), std::forward<Rest>(rest)...);
}

} // namespace details

template <typename... Args>
void read(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Args&&... args)
{
    details::read_grouping(intf, m, std::make_tuple(),
                           std::forward<Args>(args)...);
}

} // namespace message

} // namespace sdbusplus
