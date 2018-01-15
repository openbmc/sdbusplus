#pragma once

#include <utility>

namespace sdbusplus
{
namespace server
{
namespace binding
{
namespace details
{

/** Utility for converting C++ types prior to 'append'ing to a message.
 *
 *  Some C++ types require conversion to a native dbus type before they
 *  can be inserted into a message.  This template provides a general no-op
 *  implementation for all other types.
 */
template <typename T> T &&convertForMessage(T &&t)
{
    return std::forward<T>(t);
}
} // namespace details
} // namespace binding
} // namespace server
} // namespace sdbusplus
