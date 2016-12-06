#pragma once

#include <string>
#include <sdbusplus/bus.hpp>

namespace sdbusplus
{
namespace server
{
namespace transaction
{
namespace details
{

    /** @brief Generate transaction id from hashigh the  bus name
      * and message cookie.
      *
      * @param[in] bus - Dbus bus
      * @param[in] msg - Dbus message
      * @return The generated transaction id.
      */
    auto generate_id(sdbusplus::bus::bus& bus,
                     sdbusplus::message::message& msg)
    {
        const char* uniqueName = bus.get_unique_name();

        // Convert const char* to std::string since std::hash does not
        // have support for char
        std::string uniqueNameStr(uniqueName, 0, strlen(uniqueName));

        size_t hashName = std::hash<std::string>{}(uniqueNameStr);

        uint64_t cookie = msg.get_cookie();
        size_t hashCookie = std::hash<uint64_t>{}(cookie);

        // Combine the hash for the unique name and cookie following the
        // algorithm from boost::hash_combine() (Documented in the boost lib)
        hashName^= hashCookie + 0x9e3779b9 + (hashName << 6) + (hashName >> 2);

        return hashName;
    }

} // details
} // transaction
} // server
} // sdbusplus
