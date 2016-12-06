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

        auto hashName = std::hash<std::string>{}(uniqueName);
        auto hashCookie = std::hash<uint64_t>{}(cookie);

        // Combine the hash for the unique name and cookie following the
        // algorithm from boost::hash_combine() (Documented in the boost lib)
        hashName^= hashCookie + 0x9e3779b9 + (hashName << 6) + (hashName >> 2);

        return hashName;
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

    /** @brief Get transaction id
      *
      * @return The value of the transaction id
      */
    uint64_t get_id()
    {
        return details::transactionId;
    }

} // transaction
} // server
} // sdbusplus
