#pragma once

#include "fixed_string.hpp"

#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <string>
#include <string_view>

namespace sdbusplus
{

namespace message
{

// as per
// https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-marshaling-object-path
template <fixed_string S>
inline constexpr bool isValidObjectPath()
{
    // get rid of the null terminator
    std::string_view view = S.view();

    if (view.size() == 0)
    {
        return false;
    }

    // The path must begin with an ASCII '/' (integer 47) character
    if (view[0] != '/')
    {
        return false;
    }

    if (view.size() > 1 && view[view.size() - 1] == '/')
    {
        return false;
    }

    bool prevSlash = true;

    for (size_t i = 1; i < view.size(); i++)
    {
        if (view[i] == '/')
        {
            if (prevSlash)
            {
                // Multiple '/' characters cannot occur in sequence.
                return false;
            }
            prevSlash = true;
            continue;
        }
        if (sdbusplus::message::details::pathRequiresEscape(view[i]))
        {
            return false;
        }
        prevSlash = false;
    }
    return true;
}

template <fixed_string S>
struct obj_path
{
    static constexpr auto literal = S;
    static constexpr std::string_view view = S.view();

    constexpr obj_path()
    {
        static_assert(isValidObjectPath<S>());
    }

    operator std::string() const
    {
        return S;
    }
    constexpr operator std::string_view() const
    {
        return S;
    }
    constexpr operator const char*() const
    {
        return S;
    }

    static constexpr std::string_view filename()
    {
        if (view == "/")
            return "";
        auto pos = view.find_last_of('/');

        if (pos == std::string_view::npos)
        {
            return "";
        }

        auto filename = view.substr(pos + 1);

        // If we don't see that this was encoded by sdbusplus, return the naive
        // version of the filename path.
        if (filename.empty() || filename[0] != '_')
        {
            return filename;
        }

        // constexpr object path does not deal with encoded object paths since
        // they only occur at runtime.
        // Did not yet encounter an encoded object path know at compile time
        return "";
    }

    static constexpr std::string_view parent_path()
    {
        if (view == "/")
            return "/";
        auto pos = view.find_last_of('/');

        if (pos == 0)
        {
            return "/";
        }

        return view.substr(0, pos);
    }

    template <fixed_string S2>
    constexpr auto operator/(sdbusplus::message::obj_path<S2> o2)
        requires(S.size() == 2)
    // special case for S == '/' , with null terminator
    {
        return o2;
    }

    template <fixed_string S2>
    constexpr auto operator/(sdbusplus::message::obj_path<S2>)
        requires(S.size() > 2)
    {
        // - we don't have a trailing '/' and our parameter has a leading '/'
        // - both are valid object paths
        // => concatenation requires no checks since left operand is not '/'

        constexpr auto fixed = S + S2;

        return sdbusplus::message::obj_path<fixed>();
    }

    constexpr std::string operator/(std::string_view extId) const
    {
        std::string str(S);

        str.reserve(str.size() + 1 + extId.size() * 3);
        if (!str.empty() && str[str.size() - 1] != '/')
        {
            str.append(1, '/');
        }
        if (extId.empty() || (!details::pathShouldEscape(extId[0]) &&
                              std::none_of(extId.begin() + 1, extId.end(),
                                           details::pathRequiresEscape)))
        {
            str.append(extId);
            return str;
        }
        details::pathAppendEscape(str, extId[0]);
        for (auto c : extId.substr(1))
        {
            if (details::pathShouldEscape(c))
            {
                details::pathAppendEscape(str, c);
            }
            else
            {
                str.append(1, c);
            }
        }
        return str;
    }

    friend constexpr bool operator==(std::string_view lhs, const obj_path&)
    {
        return lhs == view;
    }

    constexpr bool operator==(const std::string_view v)
    {
        return view == v;
    }
    constexpr bool operator==(const std::string& v)
    {
        return view == std::string_view(v);
    }
    template <fixed_string S2>
    constexpr bool operator==(const sdbusplus::message::obj_path<S2>& _)
    {
        return S == S2;
    }
};

} // namespace message

} // namespace sdbusplus
