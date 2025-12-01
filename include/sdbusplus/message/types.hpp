#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/utility/container_traits.hpp>
#include <sdbusplus/utility/type_traits.hpp>

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace sdbusplus
{

namespace message
{

namespace types
{

/** @fn type_id()
 *  @brief Get a tuple containing the dbus type character(s) for a
 *         sequence of C++ types.
 *
 *  @tparam Args - Types to get the type_id sequence for.
 *  @returns A tuple of characters representing the dbus types for ...Args.
 *
 *  The design uses a tuple because a tuple can, at compile-time, be easily
 *  appended to, concatenated, and converted to an array (representing a
 *  C-string).  There are other options to create a string of characters but
 *  they require runtime processing and memory allocation.  By performing all
 *  options at compile-time the use of type-deduced dbus strings is equal to
 *  the cost of hard-coded type string constants.
 */
template <typename... Args>
constexpr auto type_id();

namespace details
{

/** @brief Downcast type submembers.
 *
 * This allows std::tuple and std::pair members to be downcast to their
 * non-const, nonref versions of themselves to limit duplication in template
 * specializations
 *
 *  1. Remove references.
 *  2. Remove 'const' and 'volatile'.
 *  3. Convert 'char[N]' to 'char*'.
 */
template <typename T>
struct downcast_members
{
    using type = T;
};
template <typename... Args>
struct downcast_members<std::pair<Args...>>
{
    using type =
        std::pair<utility::array_to_ptr_t<char, std::decay_t<Args>>...>;
};

template <typename... Args>
struct downcast_members<std::tuple<Args...>>
{
    using type =
        std::tuple<utility::array_to_ptr_t<char, std::decay_t<Args>>...>;
};

template <typename T>
using downcast_members_t = typename downcast_members<T>::type;

/** @brief Convert some C++ types to others for 'type_id' conversion purposes.
 *
 *  Similar C++ types have the same dbus type-id, so 'downcast' those to limit
 *  duplication in type_id template specializations.
 *
 *  1. Remove references.
 *  2. Remove 'const' and 'volatile'.
 *  3. Convert 'char[N]' to 'char*'.
 */
template <typename T>
struct type_id_downcast
{
    using type =
        utility::array_to_ptr_t<char, downcast_members_t<std::decay_t<T>>>;
};

template <typename T>
using type_id_downcast_t = typename type_id_downcast<T>::type;

/** @struct null_type_id
 *  @brief Special type indicating the dbus-type is skipped for a C++ type.
 */
struct null_type_id
{
    static constexpr std::string value()
    {
        return "";
    }
};

/** @struct undefined_type_id
 *  @brief Special type indicating no dbus-type_id is defined for a C++ type.
 */
struct undefined_type_id : public null_type_id
{};

/** @brief Special type indicating a single character string of dbus-type_id's.
 *
 *  @tparam C - The dbus type character character.
 */
template <char C>
struct type_id_inherit
{
    static constexpr std::string value()
    {
        return std::string(1, C);
    }
};

/** @fn type_id_single()
 *  @brief Get a tuple containing the dbus type character(s) for a C++ type.
 *
 *  @tparam T - The type to get the dbus type character(s) for.
 */
template <typename T>
constexpr auto type_id_single();

/** @brief Defined dbus type tuple for a C++ type.
 *
 *  @tparam T - C++ type.
 *
 *  Struct must have a 'value' tuple containing the dbus type.  The default
 *  value is an empty tuple, which is used to indicate an unsupported type.
 *
 *  Enums are converted to strings on the dbus by some special conversion
 *  routines.
 */
template <typename T, typename Enable = void>
struct type_id : undefined_type_id
{};

template <typename... Args>
inline constexpr auto type_id_v = []() { return type_id<Args...>::value(); }();

// Specializations for built-in types.
template <>
struct type_id<bool> : type_id_inherit<SD_BUS_TYPE_BOOLEAN>
{};
template <>
struct type_id<uint8_t> : type_id_inherit<SD_BUS_TYPE_BYTE>
{};
// int8_t isn't supported by dbus.
template <>
struct type_id<uint16_t> : type_id_inherit<SD_BUS_TYPE_UINT16>
{};
template <>
struct type_id<int16_t> : type_id_inherit<SD_BUS_TYPE_INT16>
{};
template <>
struct type_id<uint32_t> : type_id_inherit<SD_BUS_TYPE_UINT32>
{};
template <>
struct type_id<int32_t> : type_id_inherit<SD_BUS_TYPE_INT32>
{};
template <>
struct type_id<uint64_t> : type_id_inherit<SD_BUS_TYPE_UINT64>
{};
template <>
struct type_id<int64_t> : type_id_inherit<SD_BUS_TYPE_INT64>
{};
// float isn't supported by dbus.
template <>
struct type_id<double> : type_id_inherit<SD_BUS_TYPE_DOUBLE>
{};
template <>
struct type_id<const char*> : type_id_inherit<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<char*> : type_id_inherit<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<unix_fd> : type_id_inherit<SD_BUS_TYPE_UNIX_FD>
{};
template <>
struct type_id<std::string> : type_id_inherit<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<std::string_view> : type_id_inherit<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<object_path> : type_id_inherit<SD_BUS_TYPE_OBJECT_PATH>
{};
template <>
struct type_id<signature> : type_id_inherit<SD_BUS_TYPE_SIGNATURE>
{};
template <>
struct type_id<void> : null_type_id
{};
template <>
struct type_id<std::monostate> : null_type_id
{};

template <utility::is_dbus_enum T>
struct type_id<T> : type_id_inherit<SD_BUS_TYPE_STRING>
{};

template <typename... Args>
struct type_id<std::variant<Args...>> : type_id_inherit<SD_BUS_TYPE_VARIANT>
{};

template <utility::is_dbus_array T>
struct type_id<T>
{
    static constexpr std::string value()
    {
        std::string s(1, SD_BUS_TYPE_ARRAY);
        s += type_id<type_id_downcast_t<typename T::value_type>>::value();
        return s;
    };
};

template <typename T1, typename T2>
struct type_id<std::pair<T1, T2>>
{
    static constexpr std::string value()
    {
        std::string s(1, SD_BUS_TYPE_DICT_ENTRY_BEGIN);
        s += type_id<type_id_downcast_t<T1>>::value();
        s += type_id<type_id_downcast_t<T2>>::value();
        s += SD_BUS_TYPE_DICT_ENTRY_END;
        return s;
    }
};

template <typename... Args>
struct type_id<std::tuple<Args...>>
{
    static constexpr std::string value()
    {
        std::string s(1, SD_BUS_TYPE_STRUCT_BEGIN);
        ((s += type_id<type_id_downcast_t<Args>>::value()), ...);
        s += SD_BUS_TYPE_STRUCT_END;
        return s;
    }
};

template <typename T>
constexpr auto type_id_single()
{
    static_assert(!std::is_base_of_v<undefined_type_id, type_id<T>>,
                  "No dbus type conversion provided for type.");
    return type_id<T>::value();
}

template <typename T, typename... Args>
constexpr auto type_id_multiple()
{
    std::string s{};
    s += type_id_single<T>();
    ((s += type_id_single<Args>()), ...);
    return s;
}

template <typename T, std::size_t... I>
constexpr auto type_id_tuple(std::index_sequence<I...>)
{
    return type_id_multiple<
        type_id_downcast_t<std::tuple_element_t<I, T>>...>();
}

} // namespace details

template <typename... Args>
constexpr auto type_id()
{
    return details::type_id_multiple<details::type_id_downcast_t<Args>...>();
}

// Special case for empty type.
template <>
constexpr auto type_id()
{
    return std::string{};
}

template <typename T>
constexpr auto type_id_tuple()
{
    if constexpr (std::tuple_size_v<T> == 0)
    {
        return std::string{};
    }
    else
    {
        return details::type_id_tuple<T>(
            std::make_index_sequence<std::tuple_size_v<T>>());
    }
}

} // namespace types

} // namespace message

} // namespace sdbusplus
