#pragma once

#include <string>

namespace sdbusplus
{

namespace message
{

namespace details
{

/** Simple wrapper class for std::string to allow conversion to and from an
 *  alternative typename. */
template <typename T> struct string_wrapper
{
    std::string str;

    string_wrapper() = default;
    string_wrapper(const string_wrapper &) = default;
    string_wrapper &operator=(const string_wrapper &) = default;
    string_wrapper(string_wrapper &&) = default;
    string_wrapper &operator=(string_wrapper &&) = default;
    ~string_wrapper() = default;

    string_wrapper(const std::string &str) : str(str)
    {
    }
    string_wrapper(std::string &&str) : str(std::move(str))
    {
    }

    operator std::string() const
    {
        return str;
    }
    operator const std::string &() const
    {
        return str;
    }
    operator std::string &&()
    {
        return std::move(str);
    }

    bool operator==(const string_wrapper<T> &r) const
    {
        return str == r.str;
    }
    bool operator<(const string_wrapper<T> &r) const
    {
        return str < r.str;
    }
    bool operator==(const std::string &r) const
    {
        return str == r;
    }
    bool operator<(const std::string &r) const
    {
        return str < r;
    }

    friend bool operator==(const std::string &l, const string_wrapper &r)
    {
        return l == r.str;
    }
    friend bool operator<(const std::string &l, const string_wrapper &r)
    {
        return l < r.str;
    }
};

/** Typename for sdbus OBJECT_PATH types. */
struct object_path_type
{
};
/** Typename for sdbus SIGNATURE types. */
struct signature_type
{
};

} // namespace details

/** std::string wrapper for OBJECT_PATH. */
using object_path = details::string_wrapper<details::object_path_type>;
/** std::string wrapper for SIGNATURE. */
using signature = details::string_wrapper<details::signature_type>;

} // namespace message
} // namespace sdbusplus

namespace std
{

/** Overload of std::hash for details::string_wrappers */
template <typename T>
struct hash<sdbusplus::message::details::string_wrapper<T>>
{
    using argument_type = sdbusplus::message::details::string_wrapper<T>;
    using result_type = std::size_t;

    result_type operator()(argument_type const &s) const
    {
        return hash<std::string>()(s.str);
    }
};

} // namespace std
