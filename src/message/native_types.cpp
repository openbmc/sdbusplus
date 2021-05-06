#include <systemd/sd-bus.h>

#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/utility/memory.hpp>

#include <array>

namespace sdbusplus
{
namespace message
{
namespace details
{

std::string string_path_wrapper::filename() const
{
    size_t firstIndex = str.rfind('/');

    // Dbus paths must start with /, if we don't find one, it's an error
    if (firstIndex == std::string::npos)
    {
        return "";
    }
    firstIndex++;
    // If we don't see that this was encoded by sdbusplus, return the naive
    // version of the filename path.
    const char* filename = str.c_str() + firstIndex;
    if (*filename != '_')
    {
        return std::string(filename);
    }

    _cleanup_free_ char* out = nullptr;
    int r = sd_bus_path_decode_many(filename, "%", &out);
    if (r <= 0)
    {
        return "";
    }
    return std::string(out);
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

string_path_wrapper string_path_wrapper::operator/(const char* extId) const
{
    string_path_wrapper out;
    _cleanup_free_ char* encOut = nullptr;
    int ret = sd_bus_path_encode(str.c_str(), extId, &encOut);
    if (ret < 0)
    {
        return out;
    }
    out.str = encOut;

    size_t firstIndex = str.size();
    if (str != "/")
    {
        firstIndex++;
    }

    // Attempt to encode the first character of the path.  This allows the
    // filename() method to "detect" that this is a path that's been encoded
    // and to decode it properly.  This was needed to support a number of
    // paths that currently dont' have any encoding, and utilize underscores
    // Ideally this, and the equivalent code in filename() would go away
    // when all paths are being encoded per systemds methods.
    if (out.str[firstIndex] == '_')
    {
        return out;
    }

    constexpr std::array<char, 16> hex{'0', '1', '2', '3', '4', '5', '6', '7',
                                       '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    uint8_t firstChar = static_cast<uint8_t>(*extId);
    out.str[firstIndex] = '_';
    std::array<char, 2> encoded{hex[(firstChar >> 4) & 0xF],
                                hex[firstChar & 0xF]};
    out.str.insert(out.str.begin() + firstIndex + 1, encoded.begin(),
                   encoded.end());

    return out;
}

string_path_wrapper& string_path_wrapper::operator/=(const char* extId)
{
    string_path_wrapper out = this->operator/(extId);
    this->str = std::move(out.str);
    return *this;
}

} // namespace details
} // namespace message
} // namespace sdbusplus
