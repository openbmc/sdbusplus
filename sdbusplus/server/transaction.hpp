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

    /** @brief Combine 2 hash values together.
      * @details Use the boost::hash_combine() algorithm.
      */
    inline auto combine_hash(auto hash1, auto hash2)
    {
        return (hash1 ^ hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2))
    }

    /** @brief Generate a transaction id from hashing the bus name
      * and message cookie.
      *
      * @param[in] bus - Dbus bus
      * @param[in] msg - Dbus message
      * @return The generated transaction id.
      */
    auto generate_id(sdbusplus::bus::bus& bus,
                     sdbusplus::message::message& msg)
    {
        auto uniqueName = bus.get_unique_name();
        auto cookie = msg.get_cookie();

        // TODO Handle errors from sdbusplus once they're available. If bus
        // or msg return an error, generate a random id.

        auto hashName = std::hash<std::string>{}(uniqueName);
        auto hashCookie = std::hash<uint64_t>{}(cookie);

        return combine_hash(hashName, hashCookie);
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

        return combine_hash(hash1, hash2);
    }

} // details

    /** @brief Get transaction id
      *
      * @return The value of the transaction id
      */
    uint64_t get_id()
    {
        if (!details::transactionId)
        {
            auto random = details::generate_random_id();
            details::transactionId = random;
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
        // TODO Call this function from within sdbusplu
        // when a dbus message is created

        auto value = details::generate_id(bus, msg);
        details::transactionId = value;
    }

} // transaction
} // server
} // sdbusplus
