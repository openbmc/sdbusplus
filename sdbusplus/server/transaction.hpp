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
    Transaction(): bus(nullptr), msg(nullptr) {}
    explicit Transaction(sdbusplus::bus::bus &bus, sdbusplus::message::message&
        msg): bus(&bus), msg(&msg) {}

    sdbusplus::bus::bus *bus;
    sdbusplus::message::message *msg;
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
    auto operator()
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
    auto operator()
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
    auto operator()
        (sdbusplus::server::transaction::Transaction const& t) const
    {
        auto hash1 = 0;
        auto hash2 = 0;

        if (t.bus && t.msg)
        {
            hash1 = std::hash<sdbusplus::bus::bus>{}(*t.bus);
            hash2 = std::hash<sdbusplus::message::message>{}(*t.msg);
        }

        // boost::hash_combine() algorithm.
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
    }
};

} // namespace std
