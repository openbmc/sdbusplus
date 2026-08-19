#pragma once

#include <nlohmann/json_fwd.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace sdbusplus
{

namespace message
{

namespace details
{

/** Simple wrapper class for std::string to allow conversion to and from an
 *  alternative typename. */
struct string_wrapper
{
    std::string str;

    string_wrapper() = default;
    string_wrapper(const string_wrapper&) = default;
    string_wrapper& operator=(const string_wrapper&) = default;
    string_wrapper(string_wrapper&&) = default;
    string_wrapper& operator=(string_wrapper&&) = default;
    ~string_wrapper() = default;

    string_wrapper(const std::string& str_in) : str(str_in) {}
    string_wrapper(std::string&& str_in) : str(std::move(str_in)) {}

    operator const std::string&() const volatile&
    {
        return const_cast<const string_wrapper*>(this)->str;
    }
    operator std::string&&() &&
    {
        return std::move(str);
    }

    bool operator==(const string_wrapper& r) const
    {
        return str == r.str;
    }
    bool operator!=(const string_wrapper& r) const
    {
        return str != r.str;
    }
    bool operator<(const string_wrapper& r) const
    {
        return str < r.str;
    }
    bool operator==(const std::string& r) const
    {
        return str == r;
    }
    bool operator!=(const std::string& r) const
    {
        return str != r;
    }
    bool operator<(const std::string& r) const
    {
        return str < r;
    }

    friend bool operator==(const std::string& l, const string_wrapper& r)
    {
        return l == r.str;
    }
    friend bool operator!=(const std::string& l, const string_wrapper& r)
    {
        return l != r.str;
    }
    friend bool operator<(const std::string& l, const string_wrapper& r)
    {
        return l < r.str;
    }
};

} // namespace details
} // namespace message

/** Simple wrapper class for std::string to allow conversion to and from an
 *  alternative typename. */
struct object_path
{
    // direct access to str is deprecated; use string() instead
    std::string str;

    // deprecated: does not create a valid D-Bus object path.
    // New code should use the other constructors.
    object_path() = default;

    object_path(const object_path&) = default;
    object_path& operator=(const object_path&) = default;
    object_path(object_path&&) = default;
    object_path& operator=(object_path&&) = default;
    ~object_path() = default;

    object_path(const std::string& str_in) : str(str_in) {}
    object_path(std::string&& str_in) : str(std::move(str_in)) {}

    template <typename Base, typename... Args>
    inline object_path(const Base& base, Args&&... args)
        requires(sizeof...(Args) >= 1)
    {
        object_path res = {base};
        ((res /= std::forward<Args>(args)), ...);
        str = std::move(res.str);
    }

    operator const std::string&() const volatile&
    {
        return const_cast<const object_path*>(this)->str;
    }
    operator std::string&&() &&
    {
        return std::move(str);
    }

    bool operator==(const object_path& r) const
    {
        return str == r.str;
    }
    bool operator!=(const object_path& r) const
    {
        return str != r.str;
    }
    bool operator<(const object_path& r) const
    {
        return str < r.str;
    }
    bool operator==(const std::string& r) const
    {
        return str == r;
    }
    bool operator!=(const std::string& r) const
    {
        return str != r;
    }
    bool operator<(const std::string& r) const
    {
        return str < r;
    }

    friend bool operator==(const std::string& l, const object_path& r)
    {
        return l == r.str;
    }
    friend bool operator!=(const std::string& l, const object_path& r)
    {
        return l != r.str;
    }
    friend bool operator<(const std::string& l, const object_path& r)
    {
        return l < r.str;
    }

    std::string filename() const;
    object_path parent_path() const;
    object_path operator/(std::string_view) const;
    object_path& operator/=(std::string_view);

    // This should not be used in new code. Use filename() or parent_path()
    // appropriately
    std::string string() const;
};

void to_json(nlohmann::json& j, const object_path& s);
void from_json(const nlohmann::json& j, object_path& s);

namespace message
{

namespace details
{

/** Typename for sdbus SIGNATURE types. */
struct signature_type
{};
/** Typename for sdbus UNIX_FD types. */
struct unix_fd_type
{
    int fd;

    unix_fd_type() = default;
    unix_fd_type(int f) : fd(f) {}

    operator int() const
    {
        return fd;
    }
};

} // namespace details

/** std::string wrapper for SIGNATURE. */
using signature = details::string_wrapper;
using unix_fd = details::unix_fd_type;

namespace details
{

void to_json(nlohmann::json& j, const string_wrapper& s);
void from_json(const nlohmann::json& j, string_wrapper& s);

template <typename T>
struct convert_from_string
{
    static auto op(const std::string&) noexcept = delete;
};

template <typename T>
struct convert_to_string
{
    static std::string op(T) = delete;
};

// Concept to determine if convert_from_string exists for a type.
template <typename T>
concept has_convert_from_string = requires(
    const std::string& str) { convert_from_string<std::decay_t<T>>::op(str); };

template <typename T>
inline constexpr bool has_convert_from_string_v = has_convert_from_string<T>;

// Concept to determine if convert_to_string exists for a type.
template <typename T>
concept has_convert_to_string =
    requires(T t) { convert_to_string<std::decay_t<T>>::op(t); };

template <typename T>
inline constexpr bool has_convert_to_string_v = has_convert_to_string<T>;

} // namespace details

/** @brief Convert from a string to a native type.
 *
 *  Some C++ types cannot be represented directly on dbus, so we encode
 *  them as strings.  Enums are the primary example of this.  This is a
 *  template function prototype for the conversion from string functions.
 *
 *  @return A std::optional<T> containing the value if conversion is possible.
 */
template <typename T>
    requires details::has_convert_from_string<T>
auto convert_from_string(const std::string& str) noexcept
{
    return details::convert_from_string<T>::op(str);
}

/** @brief Convert from a native type to a string.
 *
 *  Some C++ types cannot be represented directly on dbus, so we encode
 *  them as strings.  Enums are the primary example of this.  This is a
 *  template function prototype for the conversion to string functions.
 *
 *  @return A std::string containing an encoding of the value, if conversion is
 *          possible.
 */
template <typename T>
    requires details::has_convert_to_string<T>
std::string convert_to_string(T t)
{
    return details::convert_to_string<T>::op(t);
}

namespace details
{
// Specialization of 'convert_from_string' for variant.
template <typename... Types>
struct convert_from_string<std::variant<Types...>>
{
    static auto op(const std::string& str)
        -> std::optional<std::variant<Types...>>
    {
        if constexpr (0 < sizeof...(Types))
        {
            return process<Types...>(str);
        }
        return {};
    }

    // We need to iterate through all the variant types and find
    // the one which matches the contents of the string.  Often,
    // a variant can contain both a convertible-type (ie. enum) and
    // a string, so we need to iterate through all the convertible-types
    // first and convert to string as a last resort.
    template <typename T, typename... Args>
    static auto process(const std::string& str)
        -> std::optional<std::variant<Types...>>
    {
        // If convert_from_string exists for the type, attempt it.
        if constexpr (has_convert_from_string_v<T>)
        {
            auto r = convert_from_string<T>::op(str);
            if (r)
            {
                return r;
            }
        }

        // If there are any more types in the variant, try them.
        if constexpr (0 < sizeof...(Args))
        {
            auto r = process<Args...>(str);
            if (r)
            {
                return r;
            }
        }

        // Otherwise, if this is a string, do last-resort conversion.
        if constexpr (std::is_same_v<std::string, std::remove_cv_t<T>>)
        {
            return str;
        }

        return {};
    }
};

} // namespace details

/** Export template helper to determine if a type has convert_from_string. */
template <typename T>
inline constexpr bool has_convert_from_string_v =
    details::has_convert_from_string_v<T>;

/** Export template helper to determine if a type has convert_to_string. */
template <typename T>
inline constexpr bool has_convert_to_string_v =
    details::has_convert_to_string_v<T>;

} // namespace message

} // namespace sdbusplus

namespace std
{

/** Overload of std::hash for details::string_wrappers */
template <>
struct hash<sdbusplus::message::details::string_wrapper>
{
    using argument_type = sdbusplus::message::details::string_wrapper;
    using result_type = std::size_t;

    result_type operator()(const argument_type& s) const
    {
        return hash<std::string>()(s.str);
    }
};

/** Overload of std::hash for details::string_wrappers */
template <>
struct hash<sdbusplus::object_path>
{
    using argument_type = sdbusplus::object_path;
    using result_type = std::size_t;

    result_type operator()(const argument_type& s) const
    {
        return hash<std::string>()(s.str);
    }
};

} // namespace std
