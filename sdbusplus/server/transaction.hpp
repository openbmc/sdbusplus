#pragma once

#include <sdbusplus/bus.hpp>

namespace sdbusplus
{
namespace server
{
namespace transaction
{

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

        // boost::hash_combine() algorithm.
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

} // namespace std
