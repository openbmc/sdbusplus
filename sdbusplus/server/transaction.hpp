#pragma once

#include <string>
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

        // boost::hash_combine() algorithm.
        return hash1 ^ (hash2 + 0x9e3779b9 + (hash2 << 6) + (hash2 >> 2));
    }
};

} // namespace std
