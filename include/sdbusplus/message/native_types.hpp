#pragma once

#include <optional>
#include <string>
#include <string_view>
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

/** Simple wrapper class for std::string to allow conversion to and from an
 *  alternative typename. */
struct string_path_wrapper
{
    std::string str;

    string_path_wrapper() = default;
    string_path_wrapper(const string_path_wrapper&) = default;
    string_path_wrapper& operator=(const string_path_wrapper&) = default;
    string_path_wrapper(string_path_wrapper&&) = default;
    string_path_wrapper& operator=(string_path_wrapper&&) = default;
    ~string_path_wrapper() = default;

    string_path_wrapper(const std::string& str_in) : str(str_in) {}
    string_path_wrapper(std::string&& str_in) : str(std::move(str_in)) {}

    operator const std::string&() const volatile&
    {
        return const_cast<const string_path_wrapper*>(this)->str;
    }
    operator std::string&&() &&
    {
        return std::move(str);
    }

    bool operator==(const string_path_wrapper& r) const
    {
        return str == r.str;
    }
    bool operator!=(const string_path_wrapper& r) const
    {
        return str != r.str;
    }
    bool operator<(const string_path_wrapper& r) const
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

    friend bool operator==(const std::string& l, const string_path_wrapper& r)
    {
        return l == r.str;
    }
    friend bool operator!=(const std::string& l, const string_path_wrapper& r)
    {
        return l != r.str;
    }
    friend bool operator<(const std::string& l, const string_path_wrapper& r)
    {
        return l < r.str;
    }

    std::string filename() const;
    string_path_wrapper parent_path() const;
    string_path_wrapper operator/(std::string_view) const;
    string_path_wrapper& operator/=(std::string_view);
};

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

/** std::string wrapper for OBJECT_PATH. */
using object_path = details::string_path_wrapper;
/** std::string wrapper for SIGNATURE. */
using signature = details::string_wrapper;
using unix_fd = details::unix_fd_type;

namespace details
{

// std::isalnum is not constexpr
// (assume that's because it depends on locale).
// https://en.cppreference.com/w/cpp/string/byte/isalnum.html
constexpr int isalnumConstexpr(char c)
{
    if (c >= 'a' && c <= 'z')
    {
        return 1;
    }
    if (c >= 'A' && c <= 'Z')
    {
        return 1;
    }
    if (c >= '0' && c <= '9')
    {
        return 1;
    }
    return 0;
}

constexpr bool pathShouldEscape(char c)
{
    return !isalnumConstexpr(c);
}

constexpr bool pathRequiresEscape(char c)
{
    return pathShouldEscape(c) && c != '_';
}
void pathAppendEscape(std::string& s, char c);

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
std::string convert_to_string(T t)
{
    return details::convert_to_string<T>::op(t);
}

namespace details
{
// SFINAE templates to determine if convert_from_string exists for a type.
template <typename T>
auto has_convert_from_string_helper(T)
    -> decltype(convert_from_string<T>::op(std::declval<std::string>()),
                std::true_type());
auto has_convert_from_string_helper(...) -> std::false_type;

template <typename T>
struct has_convert_from_string :
    decltype(has_convert_from_string_helper(std::declval<T>())){};

template <typename T>
inline constexpr bool has_convert_from_string_v =
    has_convert_from_string<T>::value;

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
struct hash<sdbusplus::message::details::string_path_wrapper>
{
    using argument_type = sdbusplus::message::details::string_path_wrapper;
    using result_type = std::size_t;

    result_type operator()(const argument_type& s) const
    {
        return hash<std::string>()(s.str);
    }
};

} // namespace std
