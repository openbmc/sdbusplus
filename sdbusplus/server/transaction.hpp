#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/bus.hpp>

#include <chrono>
#include <stdexcept>
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
    {}

    int time;
    std::thread::id thread;
};

} // namespace details

struct Transaction
{
    Transaction(sdbusplus::bus::bus& bus, sdbusplus::message::message& msg) :
        bus(bus), msg(msg)
    {}

    sdbusplus::bus::bus& bus;
    sdbusplus::message::message& msg;
};

} // namespace transaction
} // namespace server
} // namespace sdbusplus

namespace std
{

/** @ brief Overload of std::hash for sdbusplus::bus::bus */
template <>
struct hash<sdbusplus::bus::bus>
{
    auto operator()(sdbusplus::bus::bus& b) const
    {
        auto name = b.get_unique_name();
        return std::hash<std::string>{}(name);
    }
};

/** @ brief Overload of std::hash for sdbusplus::message::message */
template <>
struct hash<sdbusplus::message::message>
{
    auto operator()(sdbusplus::message::message& m) const
    {
        switch (m.get_type())
        {
            // Reply messages will always embed the cookie of the original
            // message in a separate location. We want to use this cookie
            // to correlate messages as one transaction.
            case SD_BUS_MESSAGE_METHOD_RETURN:
            case SD_BUS_MESSAGE_METHOD_ERROR:
                return std::hash<uint64_t>{}(m.get_reply_cookie());
            // Method calls will have the cookie in the header when sealed.
            // Since we are on the server side that should always be the case.
            case SD_BUS_MESSAGE_METHOD_CALL:
                return std::hash<uint64_t>{}(m.get_cookie());
            // Outgoing signals don't have a cookie so we need to use
            // something else as an id. Just use a monotonic unique one.
            case SD_BUS_MESSAGE_SIGNAL:
                return std::hash<uint64_t>{}(std::chrono::steady_clock::now()
                                                 .time_since_epoch()
                                                 .count());
            default:
                throw std::runtime_error("hash message: Unknown message type");
        }
    }
};

/** @ brief Overload of std::hash for Transaction */
template <>
struct hash<sdbusplus::server::transaction::Transaction>
{
    auto operator()(sdbusplus::server::transaction::Transaction const& t) const
    {
        auto hash1 = std::hash<sdbusplus::bus::bus>{}(t.bus);
        auto hash2 = std::hash<sdbusplus::message::message>{}(t.msg);

        // boost::hash_combine() algorithm.
        return static_cast<size_t>(
            hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2)));
    }
};

/** @ brief Overload of std::hash for details::Transaction */
template <>
struct hash<sdbusplus::server::transaction::details::Transaction>
{
    auto operator()(
        sdbusplus::server::transaction::details::Transaction const& t) const
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
