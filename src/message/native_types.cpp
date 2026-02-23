#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>

namespace sdbusplus
{
namespace message
{
namespace details
{

constexpr std::array<char, 16> hex{'0', '1', '2', '3', '4', '5', '6', '7',
                                   '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

constexpr std::array<int8_t, 256> unhex = [] {
    std::array<int8_t, 256> ret{};
    for (size_t i = 0; i < ret.size(); ++i)
    {
        ret[i] = -1;
    }
    for (char i = 0; i < 10; ++i)
    {
        ret['0' + i] = i;
    }
    for (char i = 0; i < 6; ++i)
    {
        ret['A' + i] = i + 10;
        ret['a' + i] = i + 10;
    }
    return ret;
}();

inline bool pathShouldEscape(char c)
{
    return !std::isalnum(c);
}

inline bool pathRequiresEscape(char c)
{
    return pathShouldEscape(c) && c != '_';
}

inline void pathAppendEscape(std::string& s, char c)
{
    s.append(1, '_');
    s.append(1, hex[(c >> 4) & 0xf]);
    s.append(1, hex[c & 0xf]);
}

std::string string_path_wrapper::filename() const
{
    std::string_view strv(str);
    size_t firstIndex = strv.rfind('/');

    // Dbus paths must start with /, if we don't find one, it's an error
    if (firstIndex == std::string::npos)
    {
        return "";
    }
    auto filename = strv.substr(firstIndex + 1);

    // If we don't see that this was encoded by sdbusplus, return the naive
    // version of the filename path.
    if (filename.empty() || filename[0] != '_')
    {
        return std::string(filename);
    }

    std::string out;
    out.reserve(filename.size());
    for (size_t i = 0; i < filename.size(); ++i)
    {
        if (filename[i] != '_')
        {
            out.append(1, filename[i]);
            continue;
        }
        if (i + 2 >= filename.size())
        {
            return "";
        }
        auto ch = unhex[filename[i + 1]];
        auto cl = unhex[filename[i + 2]];
        if (ch == -1 || cl == -1)
        {
            return "";
        }
        out.append(1, static_cast<char>((ch << 4) | cl));
        i += 2;
    }
    return out;
}

string_path_wrapper string_path_wrapper::parent_path() const
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

string_path_wrapper string_path_wrapper::operator/(std::string_view extId) const
{
    string_path_wrapper out("");
    out.str.reserve(str.size() + 1 + extId.size() * 3);
    out.str.append(str);
    return out /= extId;
}

string_path_wrapper& string_path_wrapper::operator/=(std::string_view extId)
{
    str.reserve(str.size() + 1 + extId.size() * 3);
    if (!str.empty() && str[str.size() - 1] != '/')
    {
        str.append(1, '/');
    }
    if (extId.empty() ||
        (!pathShouldEscape(extId[0]) &&
         std::none_of(extId.begin() + 1, extId.end(), pathRequiresEscape)))
    {
        str.append(extId);
        return *this;
    }
    pathAppendEscape(str, extId[0]);
    for (auto c : extId.substr(1))
    {
        if (pathShouldEscape(c))
        {
            pathAppendEscape(str, c);
        }
        else
        {
            str.append(1, c);
        }
    }
    return *this;
}

} // namespace details
} // namespace message
} // namespace sdbusplus
