#pragma once

namespace sdbusplus::client::${interface.cppNamespacedClass()}
{

static constexpr auto interface = "${interface.name}";

} // namespace sdbusplus::client::${interface.cppNamespacedClass()}

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace sdbusplus::${interface.old_cppNamespacedClass("client")}
{

using sdbusplus::client::${interface.cppNamespacedClass()}::interface;

} // namespace sdbusplus::${interface.old_cppNamespacedClass("client")}
#endif
