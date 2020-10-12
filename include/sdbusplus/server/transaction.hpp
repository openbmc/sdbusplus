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
    size_t operator()(sdbusplus::bus::bus& b) const;
};

/** @ brief Overload of std::hash for sdbusplus::message::message */
template <>
struct hash<sdbusplus::message::message>
{
    size_t operator()(sdbusplus::message::message& m) const;
};

/** @ brief Overload of std::hash for Transaction */
template <>
struct hash<sdbusplus::server::transaction::Transaction>
{
    size_t
        operator()(sdbusplus::server::transaction::Transaction const& t) const;
};

/** @ brief Overload of std::hash for details::Transaction */
template <>
struct hash<sdbusplus::server::transaction::details::Transaction>
{
    size_t operator()(
        sdbusplus::server::transaction::details::Transaction const& t) const;
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
void set_id(message::message& msg);

} // namespace transaction
} // namespace server
} // namespace sdbusplus
