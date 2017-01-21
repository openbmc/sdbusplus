#pragma once

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
extern thread_local uint64_t id;

struct Transaction
{
    Transaction(): time(std::time(nullptr)), thread(std::this_thread::get_id())
        {}

    int time;
    std::thread::id thread;
};

} // namespace details

struct Transaction
{
    Transaction(sdbusplus::bus::bus &bus, sdbusplus::message::message& msg):
        bus(bus), msg(msg) {}

    sdbusplus::bus::bus& bus;
    sdbusplus::message::message& msg;
};

} // namespace transaction
} // namespace server
} // namespace sdbusplus

namespace std
{

/** @ brief Overload of std::hash for std::pair<size_t, size_t> */
template <>
struct hash<std::pair<size_t, size_t>>
{
    std::size_t operator()
        (std::pair<size_t, size_t> const& p) const
    {
        // boost::hash_combine() algorithm.
        return p.first ^ (p.second + 0x9e3779b9 + (p.first << 6)
            + (p.first >> 2));
    }
};

/** @ brief Overload of std::hash for sdbusplus::bus::bus */
template <>
struct hash<sdbusplus::bus::bus>
{
    std::size_t operator()
        (sdbusplus::bus::bus& b) const
    {
        auto name = b.get_unique_name();
        return std::hash<std::string>{}(name);
    }
};

/** @ brief Overload of std::hash for sdbusplus::message::message */
template <>
struct hash<sdbusplus::message::message>
{
    std::size_t operator()
        (sdbusplus::message::message& m) const
    {
        auto cookie = m.get_cookie();
        return std::hash<uint64_t>{}(cookie);
    }
};

/** @ brief Overload of std::hash for Transaction */
template <>
struct hash<sdbusplus::server::transaction::Transaction>
{
    std::size_t operator()
        (sdbusplus::server::transaction::Transaction const& t) const
    {
        std::size_t hash1 = std::hash<sdbusplus::bus::bus>{}(t.bus);
        std::size_t hash2 = std::hash<sdbusplus::message::message>{}(t.msg);

        return std::hash<std::pair<size_t, size_t>>{}
            (std::make_pair(hash1, hash2));
    }
};

/** @ brief Overload of std::hash for details::Transaction */
template <>
struct hash<sdbusplus::server::transaction::details::Transaction>
{
    std::size_t operator()
        (sdbusplus::server::transaction::details::Transaction const& t) const
    {
        std::size_t hash1 = std::hash<int>{}(t.time);
        std::size_t hash2 = std::hash<std::thread::id>{}(t.thread);

        return std::hash<std::pair<size_t, size_t>>{}
            (std::make_pair(hash1, hash2));
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
