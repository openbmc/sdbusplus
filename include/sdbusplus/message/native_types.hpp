#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/utility/memory.hpp>

#include <string>

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

    string_wrapper(const std::string& str) : str(str)
    {}
    string_wrapper(std::string&& str) : str(std::move(str))
    {}

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

    string_path_wrapper(const std::string& str) : str(str)
    {}
    string_path_wrapper(std::string&& str) : str(std::move(str))
    {}

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

    std::string filename() const
    {
        std::string parent = parent_path();
        _cleanup_free_ char* out = nullptr;
        int r = sd_bus_path_decode(str.c_str(), parent.c_str(), &out);
        if (r <= 0)
        {
            return "";
        }
        std::string ret(out);
        return ret;
    }

    string_path_wrapper parent_path() const
    {
        auto index = str.rfind('/');
        if (index == std::string::npos)
        {
            return string_path_wrapper("/");
        }
        if (index <= 1)
        {
            return string_path_wrapper("/");
        }

        return str.substr(0, index);
    }

    string_path_wrapper operator/(const std::string& extId)
    {
        return this->operator/(extId.c_str());
    }

    string_path_wrapper operator/(const char* extId)
    {
        string_path_wrapper out;
        _cleanup_free_ char* encOut = nullptr;
        int ret = sd_bus_path_encode(str.c_str(), extId, &encOut);
        if (ret < 0)
        {
            return out;
        }
        out.str = encOut;
        return out;
    }

    string_path_wrapper& operator/=(const char* extId)
    {
        string_path_wrapper out = this->operator/(extId);
        this->str = std::move(out.str);
        return *this;
    }

    string_path_wrapper& operator/=(const std::string& extId)
    {
        string_path_wrapper out = this->operator/(extId);
        this->str = std::move(out.str);
        return *this;
    }
};

/** Typename for sdbus SIGNATURE types. */
struct signature_type
{};
/** Typename for sdbus UNIX_FD types. */
struct unix_fd_type
{
    int fd;

    unix_fd_type() = default;
    unix_fd_type(int f) : fd(f)
    {}

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

    result_type operator()(argument_type const& s) const
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

    result_type operator()(argument_type const& s) const
    {
        return hash<std::string>()(s.str);
    }
};

} // namespace std
