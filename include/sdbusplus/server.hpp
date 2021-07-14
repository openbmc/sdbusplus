#pragma once

#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>
// Alias for deprecated sdbusplus::server::match namespace.
namespace sdbusplus
{
namespace server
{
namespace match = bus::match;
using match_t = bus::match_t;
} // namespace server
} // namespace sdbusplus
#include <sdbusplus/server/interface.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/server/object.hpp>
#include <sdbusplus/server/transaction.hpp>
#include <sdbusplus/vtable.hpp>
