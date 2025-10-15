#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/types.hpp>
#include <sdbusplus/utility/tuple_to_array.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <iostream>
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
 *  @param[in] m - The message to read from.
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

/** @brief Utility to read a single C++ element from a sd_bus_message.
 *
 *  User-defined types are expected to specialize this template in order to
 *  get their functionality.
 *
 *  @tparam S - Type of element to read.
 */
template <typename S>
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
     *  to allow the function to be utilized for many variants of S:
     *  S&, S&&, const S&, volatile S&, etc. The type_id_downcast is used
     *  to ensure T and S are equivalent.  For 'char*', this also allows
     *  use for 'char[N]' types.
     *
     *  @param[in] m - sd_bus_message to read from.
     *  @param[out] t - The reference to read item into.
     */
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
        requires(!std::is_enum_v<Td<T>>)
    {
        // For this default implementation, we need to ensure that only
        // basic types are used.
        static_assert(std::is_fundamental_v<Td<T>> ||
                          std::is_convertible_v<Td<T>, const char*> ||
                          std::is_convertible_v<Td<T>, details::unix_fd_type>,
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
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
        requires(std::is_enum_v<Td<T>>)
    {
        std::string value{};
        sdbusplus::message::read(intf, m, value);

        auto r = sdbusplus::message::convert_from_string<Td<T>>(value);
        if (!r)
        {
            std::cout << "    read_single: throwing InvalidEnumString for "
                      << value << '\n';
            throw sdbusplus::exception::InvalidEnumString();
        }
        t = *r;
    }
};

template <typename T>
using read_single_t = read_single<types::details::type_id_downcast_t<T>>;

/** @brief Specialization of read_single for various string class types.
 *
 *  Supports std::strings, details::string_wrapper and
 *  details::string_path_wrapper.
 */
template <typename S>
    requires(std::is_same_v<S, std::string> ||
             std::is_same_v<S, details::string_wrapper> ||
             std::is_same_v<S, details::string_path_wrapper>)
struct read_single<S>
{
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S& t)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<S>());
        const char* str = nullptr;
        int r = intf->sd_bus_message_read_basic(m, dbusType, &str);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_read_basic string");
        }
        t = S(str);
    }
};

/** @brief Specialization of read_single for bools. */
template <typename S>
    requires(std::is_same_v<S, bool>)
struct read_single<S>
{
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S& t)
    {
        constexpr auto dbusType = std::get<0>(types::type_id<S>());
        int i = 0;
        int r = intf->sd_bus_message_read_basic(m, dbusType, &i);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_read_basic bool");
        }
        t = (i != 0);
    }
};

template <typename T>
concept can_read_array_non_contigious =
    utility::has_emplace_back<T> && !utility::can_append_array_value<T>;

/** @brief Specialization of read_single for std::vectors, with elements that
 * are not an integral type.
 */
template <can_read_array_non_contigious S>
struct read_single<S>
{
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S& t)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<S>());
        int r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY,
                                                     dbusType.data() + 1);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_enter_container emplace_back_container");
        }

        while (!(r = intf->sd_bus_message_at_end(m, false)))
        {
            types::details::type_id_downcast_t<typename S::value_type> s;
            sdbusplus::message::read(intf, m, s);
            t.emplace_back(std::move(s));
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

// Determines if fallback to normal iteration and append is required (can't use
// sd_bus_message_append_array)
template <typename T>
concept CanReadArrayOneShot =
    utility::has_emplace_back<T> && utility::can_append_array_value<T>;

/** @brief Specialization of read_single for std::vectors, when elements are
 * an integral type
 */
template <CanReadArrayOneShot S>
struct read_single<S>
{
    template <typename T>
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T&& t)
    {
        size_t sizeInBytes = 0;
        const void* p = nullptr;
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<S>());
        int r =
            intf->sd_bus_message_read_array(m, dbusType[1], &p, &sizeInBytes);
        if (r < 0)
        {
            throw exception::SdBusError(-r, "sd_bus_message_read_array");
        }
        using ContainedType = typename S::value_type;
        const ContainedType* begin = static_cast<const ContainedType*>(p);
        t.insert(t.end(), begin, begin + (sizeInBytes / sizeof(ContainedType)));
    }
};

/** @brief Specialization of read_single for std::map. */
template <utility::has_emplace S>
struct read_single<S>
{
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S& t)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<S>());
        int r = intf->sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY,
                                                     dbusType.data() + 1);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_enter_container emplace_container");
        }

        while (!(r = intf->sd_bus_message_at_end(m, false)))
        {
            types::details::type_id_downcast_t<typename S::value_type> s;
            try
            {
                sdbusplus::message::read(intf, m, s);
                t.emplace(std::move(s));
            }
            catch (std::exception& e)
            {
                std::cout << "Exception while reading out of map\n";
                throw;
            }
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

/** @brief Specialization of read_single for std::tuples and std::pairs. */
template <typename S>
    requires requires(S& s) { std::get<0>(s); }
struct read_single<S>
{
    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m, S& t)
    {
        constexpr auto dbusType =
            utility::tuple_to_array(types::type_id_tuple<S>());

        // Tuples use TYPE_STRUCT, pair uses TYPE_DICT_ENTRY.
        // Use the presence of `t.first` to determine if it is a pair.
        constexpr auto tupleType = [&]() {
            if constexpr (requires { t.first; })
            {
                return SD_BUS_TYPE_DICT_ENTRY;
            }
            return SD_BUS_TYPE_STRUCT;
        }();

        int r =
            intf->sd_bus_message_enter_container(m, tupleType, dbusType.data());
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_enter_container tuple");
        }

        std::apply(
            [&](auto&... args) { sdbusplus::message::read(intf, m, args...); },
            t);

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

    template <typename T, typename T1, typename... Args1>
    static void read(sdbusplus::SdBusInterface* intf, sd_bus_message* m, T& t)
    {
        constexpr auto dbusType = utility::tuple_to_array(types::type_id<T1>());

        int r = intf->sd_bus_message_verify_type(m, SD_BUS_TYPE_VARIANT,
                                                 dbusType.data());
        if (r < 0)
        {
            throw exception::SdBusError(-r,
                                        "sd_bus_message_verify_type variant");
        }
        if (!r)
        {
            if constexpr (sizeof...(Args1) == 0)
            {
                r = intf->sd_bus_message_skip(m, "v");
                if (r < 0)
                {
                    throw exception::SdBusError(-r,
                                                "sd_bus_message_skip variant");
                }
                t = std::remove_reference_t<T>{};
            }
            else
            {
                read<T, Args1...>(intf, m, t);
            }
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
        if constexpr (std::is_enum_v<Td<T1>> ||
                      std::is_same_v<std::string, Td<T1>>)
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

            t = std::move(*ret);
        }
        else // otherwise, read it out directly.
        {
            std::remove_reference_t<T1> t1;
            sdbusplus::message::read(intf, m, t1);
            t = std::move(t1);
        }

        r = intf->sd_bus_message_exit_container(m);
        if (r < 0)
        {
            throw exception::SdBusError(
                -r, "sd_bus_message_exit_container variant");
        }
    }

    static void op(sdbusplus::SdBusInterface* intf, sd_bus_message* m,
                   std::variant<Args...>& t)
    {
        read<std::variant<Args...>, Args...>(intf, m, t);
    }
};

/** @brief Specialization of read_single for std::monostate. */
template <typename S>
    requires(std::is_same_v<S, std::monostate>)
struct read_single<S>
{
    static void op(sdbusplus::SdBusInterface*, sd_bus_message*, S&) {}
};

} // namespace details

template <typename... Args>
void read(sdbusplus::SdBusInterface* intf, sd_bus_message* m, Args&&... args)
{
    (details::read_single_t<Args>::op(intf, m, args), ...);
}

} // namespace message

} // namespace sdbusplus
