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

struct Transaction
{
    sdbusplus::bus::bus& bus;
    sdbusplus::message::message& msg;
};

struct Random
{
    Random() :
        currentTime(std::time(nullptr)), threadId(std::this_thread::get_id()) {}
    int currentTime;
    std::thread::id threadId;
};

namespace details
{

// Transaction Id
thread_local uint64_t id = 0;

/** @brief Use the boost::hash_combine() algorithm to combine 2 hash values. */
template <typename H1, typename H2>
constexpr auto combine(H1&& h1, H2&& h2)
{
    return h1 ^ (h2 + 0x9e3779b9 + (h2 << 6) + (h2 >> 2));
}

} // namespace details
} // namespace transaction
} // namespace server
} // namespace sdbusplus

namespace std
{

/** @ brief Overload of std::hash for Transaction */
template <>
struct hash<sdbusplus::server::transaction::Transaction>
{
    auto operator()
        (sdbusplus::server::transaction::Transaction const& t) const
    {
        auto uniqueName = t.bus.get_unique_name();
        auto cookie = t.msg.get_cookie();

        // TODO Handle errors from sdbusplus once they're available. If bus
        // or msg return an error, generate a random id.

        auto hash1 = std::hash<std::string>{}(uniqueName);
        auto hash2 = std::hash<uint64_t>{}(cookie);

        return sdbusplus::server::transaction::details::combine(hash1, hash2);
    }
};

/** @ brief Overload of std::hash for Random */
template <>
struct hash<sdbusplus::server::transaction::Random>
{
    auto operator()
        (sdbusplus::server::transaction::Random const& r) const
    {
        auto hash1 = std::hash<int>{}(r.currentTime);
        auto hash2 = std::hash<std::thread::id>{}(r.threadId);

        return sdbusplus::server::transaction::details::combine(hash1, hash2);
    }
};

} // namespace std

namespace sdbusplus
{
namespace server
{
namespace transaction
{

/** @brief Get transaction id
  *
  * @return The value of the transaction id
  */
uint64_t get_id()
{
    // Generate a random id if the transaction id is not set.
    if (!details::id)
    {
        Random r;
        details::id = std::hash<sdbusplus::server::transaction::Random>{}(r);
    }
    return details::id;
}

} // namespace transaction
} // namespace server
} // namespace sdbusplus

