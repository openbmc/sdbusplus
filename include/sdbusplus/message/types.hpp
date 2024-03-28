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

/** @fn type_id_nonull()
 *  @brief A non-null-terminated version of type_id.
 *
 *  This is useful when type-ids may need to be concatenated.
 */
template <typename... Args>
constexpr auto type_id_nonull();

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
    using type = std::pair<utility::array_to_ptr_t<
        char, std::remove_cv_t<std::remove_reference_t<Args>>>...>;
};

template <typename... Args>
struct downcast_members<std::tuple<Args...>>
{
    using type = std::tuple<utility::array_to_ptr_t<
        char, std::remove_cv_t<std::remove_reference_t<Args>>>...>;
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
    using type = utility::array_to_ptr_t<
        char, downcast_members_t<std::remove_cv_t<std::remove_reference_t<T>>>>;
};

template <typename T>
using type_id_downcast_t = typename type_id_downcast<T>::type;

/** @struct undefined_type_id
 *  @brief Special type indicating no dbus-type_id is defined for a C++ type.
 */
struct undefined_type_id
{
    /** An empty tuple indicating no type-characters. */
    // We want this to be tuple so that we can use operations like
    // tuple_cat on it without cascading compile failures.  There is a
    // static_assert in type_id_single to ensure this is never used, but to
    // keep the compile failures as clean as possible.
    static constexpr auto value = std::make_tuple();
};

/** @brief Special type indicating a tuple of dbus-type_id's.
 *
 *  @tparam C1 - The first dbus type character character.
 *  @tparam C - The remaining sequence of dbus type characters.
 *
 *  A tuple_type_id must be one or more characters.  The C1 template param
 *  ensures at least one is present.
 */
template <char C1, char... C>
struct tuple_type_id
{
/* This version check is required because a fix for auto is in 5.2+.
 * https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66421
 */
/** A tuple containing the type-characters. */
#if (__GNUC__ > 5) || (__GNUC__ == 5 && (__GNUC_MINOR__ >= 2))
    static constexpr auto value = std::make_tuple(C1, C...);
#else
    static constexpr decltype(std::make_tuple(C1, C...)) value =
        std::make_tuple(C1, C...);
#endif
};

template <char... Chars>
inline constexpr auto tuple_type_id_v = tuple_type_id<Chars...>::value;

/** @fn type_id_single()
 *  @brief Get a tuple containing the dbus type character(s) for a C++ type.
 *
 *  @tparam T - The type to get the dbus type character(s) for.
 */
template <typename T>
constexpr auto type_id_single();

/** @fn type_id_multiple()
 *  @brief Get a tuple containing the dbus type characters for a sequence of
 *         C++ types.
 *
 *  @tparam T - The first type to get the dbus type character(s) for.
 *  @tparam Args - The remaining types.
 */
template <typename T, typename... Args>
constexpr auto type_id_multiple();

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
struct type_id :
    public std::conditional_t<
        std::is_enum_v<T>, tuple_type_id<SD_BUS_TYPE_STRING>, undefined_type_id>
{};

template <typename... Args>
inline constexpr auto type_id_v = type_id<Args...>::value;

// Specializations for built-in types.
template <>
struct type_id<bool> : tuple_type_id<SD_BUS_TYPE_BOOLEAN>
{};
template <>
struct type_id<uint8_t> : tuple_type_id<SD_BUS_TYPE_BYTE>
{};
// int8_t isn't supported by dbus.
template <>
struct type_id<uint16_t> : tuple_type_id<SD_BUS_TYPE_UINT16>
{};
template <>
struct type_id<int16_t> : tuple_type_id<SD_BUS_TYPE_INT16>
{};
template <>
struct type_id<uint32_t> : tuple_type_id<SD_BUS_TYPE_UINT32>
{};
template <>
struct type_id<int32_t> : tuple_type_id<SD_BUS_TYPE_INT32>
{};
template <>
struct type_id<uint64_t> : tuple_type_id<SD_BUS_TYPE_UINT64>
{};
template <>
struct type_id<int64_t> : tuple_type_id<SD_BUS_TYPE_INT64>
{};
// float isn't supported by dbus.
template <>
struct type_id<double> : tuple_type_id<SD_BUS_TYPE_DOUBLE>
{};
template <>
struct type_id<const char*> : tuple_type_id<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<char*> : tuple_type_id<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<unix_fd> : tuple_type_id<SD_BUS_TYPE_UNIX_FD>
{};
template <>
struct type_id<std::string> : tuple_type_id<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<std::string_view> : tuple_type_id<SD_BUS_TYPE_STRING>
{};
template <>
struct type_id<object_path> : tuple_type_id<SD_BUS_TYPE_OBJECT_PATH>
{};
template <>
struct type_id<signature> : tuple_type_id<SD_BUS_TYPE_SIGNATURE>
{};

template <utility::is_dbus_array T>
struct type_id<T> : std::false_type
{
    static constexpr auto value =
        std::tuple_cat(tuple_type_id_v<SD_BUS_TYPE_ARRAY>,
                       type_id_v<type_id_downcast_t<typename T::value_type>>);
};

template <typename T1, typename T2>
struct type_id<std::pair<T1, T2>>
{
    static constexpr auto value = std::tuple_cat(
        tuple_type_id_v<SD_BUS_TYPE_DICT_ENTRY_BEGIN>,
        type_id_v<type_id_downcast_t<T1>>, type_id_v<type_id_downcast_t<T2>>,
        tuple_type_id_v<SD_BUS_TYPE_DICT_ENTRY_END>);
};

template <typename... Args>
struct type_id<std::tuple<Args...>>
{
    static constexpr auto value =
        std::tuple_cat(tuple_type_id_v<SD_BUS_TYPE_STRUCT_BEGIN>,
                       type_id_v<type_id_downcast_t<Args>>...,
                       tuple_type_id_v<SD_BUS_TYPE_STRUCT_END>);
};

template <typename... Args>
struct type_id<std::variant<Args...>> : tuple_type_id<SD_BUS_TYPE_VARIANT>
{};

template <>
struct type_id<void>
{
    constexpr static auto value = std::make_tuple('\0');
};

template <>
struct type_id<std::monostate>
{
    constexpr static auto value = std::make_tuple('\0');
};

template <typename T>
constexpr auto type_id_single()
{
    static_assert(!std::is_base_of_v<undefined_type_id, type_id<T>>,
                  "No dbus type conversion provided for type.");
    return type_id_v<T>;
}

template <typename T, typename... Args>
constexpr auto type_id_multiple()
{
    return std::tuple_cat(type_id_single<T>(), type_id_single<Args>()...);
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
    return std::tuple_cat(
        details::type_id_multiple<details::type_id_downcast_t<Args>...>(),
        std::make_tuple('\0') /* null terminator for C-string */);
}

// Special case for empty type.
template <>
constexpr auto type_id()
{
    return std::make_tuple('\0');
}

template <typename... Args>
constexpr auto type_id_nonull()
{
    return details::type_id_multiple<details::type_id_downcast_t<Args>...>();
}

template <typename T>
constexpr auto type_id_tuple()
{
    return std::tuple_cat(
        details::type_id_tuple<T>(
            std::make_index_sequence<
                std::tuple_size_v<details::type_id_downcast_t<T>>>()),
        std::make_tuple('\0'));
}

} // namespace types

} // namespace message

} // namespace sdbusplus
