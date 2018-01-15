#pragma once

#include <sdbusplus/bus.hpp>
#include <thread>

namespace sdbusplus
{
namespace server
{
namespace transaction
{
namespace details
{

// Transaction Id
extern thread_local uint64_t id;

struct Transaction
{
    Transaction() : time(std::time(nullptr)), thread(std::this_thread::get_id())
    {
    }

    int time;
    std::thread::id thread;
};

} // namespace details

struct Transaction
{
    Transaction(sdbusplus::bus::bus &bus, sdbusplus::message::message &msg) :
        bus(bus), msg(msg)
    {
    }

    sdbusplus::bus::bus &bus;
    sdbusplus::message::message &msg;
};

} // namespace transaction
} // namespace server
} // namespace sdbusplus

namespace std
{

/** @ brief Overload of std::hash for sdbusplus::bus::bus */
template <> struct hash<sdbusplus::bus::bus>
{
    auto operator()(sdbusplus::bus::bus &b) const
    {
        auto name = b.get_unique_name();
        return std::hash<std::string>{}(name);
    }
};

/** @ brief Overload of std::hash for sdbusplus::message::message */
template <> struct hash<sdbusplus::message::message>
{
    auto operator()(sdbusplus::message::message &m) const
    {
        auto cookie = m.get_cookie();
        return std::hash<uint64_t>{}(cookie);
    }
};

/** @ brief Overload of std::hash for Transaction */
template <> struct hash<sdbusplus::server::transaction::Transaction>
{
    auto operator()(sdbusplus::server::transaction::Transaction const &t) const
    {
        auto hash1 = std::hash<sdbusplus::bus::bus>{}(t.bus);
        auto hash2 = std::hash<sdbusplus::message::message>{}(t.msg);

        // boost::hash_combine() algorithm.
        return static_cast<size_t>(
            hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2)));
    }
};

/** @ brief Overload of std::hash for details::Transaction */
template <> struct hash<sdbusplus::server::transaction::details::Transaction>
{
    auto operator()(
        sdbusplus::server::transaction::details::Transaction const &t) const
    {
        auto hash1 = std::hash<int>{}(t.time);
        auto hash2 = std::hash<std::thread::id>{}(t.thread);

        // boost::hash_combine() algorithm.
        return static_cast<size_t>(
            hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2)));
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
inline uint64_t get_id()
{
    // If the transaction id has not been initialized, generate one.
    if (!details::id)
    {
        details::Transaction t;
        details::id = std::hash<details::Transaction>{}(t);
    }
    return details::id;
}

/** @brief Set transaction id
 *
 * @param[in] value - Desired value for the transaction id
 */
inline void set_id(uint64_t value)
{
    details::id = value;
}

} // namespace transaction
} // namespace server
} // namespace sdbusplus
