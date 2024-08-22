#include "sdbusplus/server/transaction.hpp"

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

} // namespace details

uint64_t get_id()
{
    // If the transaction id has not been initialized, generate one.
    if (!details::id)
    {
        details::Transaction t;
        details::id = std::hash<details::Transaction>{}(t);
    }
    return details::id;
}

void set_id(uint64_t value)
{
    details::id = value;
}

void set_id(message_t& msg)
{
    auto tbus = msg.get_bus();
    auto t = Transaction(tbus, msg);
    set_id(std::hash<Transaction>{}(t));
}

} // namespace transaction
} // namespace server
} // namespace sdbusplus

namespace std
{

size_t hash<sdbusplus::bus_t>::operator()(sdbusplus::bus_t& b) const
{
    auto name = b.get_unique_name();
    return std::hash<std::string>{}(name);
}

size_t hash<sdbusplus::message_t>::operator()(sdbusplus::message_t& m) const
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
            return std::hash<uint64_t>{}(
                std::chrono::steady_clock::now().time_since_epoch().count());
        default:
            throw std::runtime_error("hash message: Unknown message type");
    }
}

size_t hash<sdbusplus::server::transaction::Transaction>::operator()(
    const sdbusplus::server::transaction::Transaction& t) const
{
    auto hash1 = std::hash<sdbusplus::bus_t>{}(t.bus);
    auto hash2 = std::hash<sdbusplus::message_t>{}(t.msg);

    // boost::hash_combine() algorithm.
    return static_cast<size_t>(
        hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2)));
}

size_t hash<sdbusplus::server::transaction::details::Transaction>::operator()(
    const sdbusplus::server::transaction::details::Transaction& t) const
{
    auto hash1 = std::hash<std::time_t>{}(t.time);
    auto hash2 = std::hash<std::thread::id>{}(t.thread);

    // boost::hash_combine() algorithm.
    return static_cast<size_t>(
        hash1 ^ (hash2 + 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2)));
}
} // namespace std
