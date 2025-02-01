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

    std::time_t time;
    std::thread::id thread;
};

} // namespace details

struct Transaction
{
    Transaction(sdbusplus::bus_t& bus_in, sdbusplus::message_t& msg_in) :
        bus(bus_in), msg(msg_in)
    {}

    sdbusplus::bus_t& bus;
    sdbusplus::message_t& msg;
};

} // namespace transaction
} // namespace server
} // namespace sdbusplus

namespace std
{

/** @ brief Overload of std::hash for sdbusplus::bus_t */
template <>
struct hash<sdbusplus::bus_t>
{
    size_t operator()(sdbusplus::bus_t& b) const;
};

/** @ brief Overload of std::hash for sdbusplus::message_t */
template <>
struct hash<sdbusplus::message_t>
{
    size_t operator()(sdbusplus::message_t& m) const;
};

/** @ brief Overload of std::hash for Transaction */
template <>
struct hash<sdbusplus::server::transaction::Transaction>
{
    size_t operator()(
        const sdbusplus::server::transaction::Transaction& t) const;
};

/** @ brief Overload of std::hash for details::Transaction */
template <>
struct hash<sdbusplus::server::transaction::details::Transaction>
{
    size_t operator()(
        const sdbusplus::server::transaction::details::Transaction& t) const;
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
uint64_t get_id();

/** @brief Set transaction id
 *
 * @param[in] value - Desired value for the transaction id
 */
void set_id(uint64_t value);

/** @brief Set transaction from message.
 *
 * @param[in] msg - The message to create the transaction from.
 */
void set_id(message_t& msg);

} // namespace transaction
} // namespace server
} // namespace sdbusplus
