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

// Transaction Id
thread_local uint64_t id = 0;

/** @brief Use the boost::hash_combine() algorithm to combine 2 hash values. */
constexpr auto combine(size_t h1, size_t h2)
{
    return h1 ^ (h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

} // namespace details

/** @brief Return a unique value by combining the hash of 2 parameters */
template <typename T, typename U>
const auto Transaction(T& t, U& u)
{
    return details::combine(
        std::hash<T>{}(std::forward<T>(t)), std::hash<U>{}(std::forward<U>(u)));
}

/** @brief Specialization for sdbusplus bus and msg. */
template <>
const auto Transaction<sdbusplus::bus::bus, sdbusplus::message::message>
    (sdbusplus::bus::bus& bus, sdbusplus::message::message& msg)
{
    auto uniqueName = bus.get_unique_name();
    auto cookie = msg.get_cookie();

    // TODO Handle errors from sdbusplus once they're available. If bus
    // or msg return an error, use arbitrary values to generate the id.

    auto hash1 = std::hash<std::string>{}(uniqueName);
    auto hash2 = std::hash<uint64_t>{}(cookie);

    // boost::hash_combine() algorithm.
    return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
}

/** @brief Get transaction id
  *
  * @return The value of the transaction id
  */
uint64_t get_id()
{
    // Generate a unique id if the transaction id is not set.
    // Arbitrarily use the current time and thread id.
    if (!details::id)
    {
        auto t = std::time(nullptr);
        auto i = std::this_thread::get_id();
        details::id = Transaction(t, i);
    }
    return details::id;
}

/** @brief Set transaction id
  *
  * @param[in] value - Desired value for the transaction id
  */
void set_id(uint64_t value)
{
    details::id = value;
}

} // namespace transaction
} // namespace server
} // namespace sdbusplus

