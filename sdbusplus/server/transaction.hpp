#pragma once

#include <string>
#include <thread>
#include <sdbusplus/bus.hpp>

namespace sdbusplus
{
namespace server
{
namespace transaction
{
namespace details
{
    // Thread local storage
    thread_local uint64_t transactionId = 0;

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

    /** @brief Generate a random transaction id.
      *
      * @return The generated transaction id.
      */
    auto generate_random_id()
    {
        // Arbitrarily use the current time and thread id to generate a
        // transaction id hash.

        auto hash1 = std::hash<int>{}(std::time(nullptr));
        auto hash2 = std::hash<std::thread::id>{}(std::this_thread::get_id());

        // Combine the hashes by using the algorithm from boost::hash_combine()
        hash1^= hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2);

        return hash1;
    }

    /** @brief Set transaction id
      *
      * @param[in] value - Desired value for the transaction id
      */
    void set_id(uint64_t value)
    {
        transactionId = value;
    }

} // details

    /** @brief Get transaction id. If it is not initialized, generate a random one.
      *
      * @return The value of the transaction id
      */
    uint64_t get_id()
    {
        if (!details::transactionId)
        {
            auto random = details::generate_random_id();
            details::set_id(random);
        }

        return details::transactionId;
    }

    /** @brief Generate and save the transaction id info
      *
      * @param[in] bus - Dbus bus
      * @param[in] msg - Dbus message
      */
    void enter(sdbusplus::bus::bus& bus,
               sdbusplus::message::message& msg)
    {
        auto value = details::generate_id(bus, msg);
        details::set_id(value);
    }

} // transaction
} // server
} // sdbusplus
