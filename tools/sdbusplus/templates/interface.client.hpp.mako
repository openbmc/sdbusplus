#pragma once

namespace sdbusplus::bindings::client::${interface.cppNamespacedClass()}
{

static constexpr auto interface = "${interface.name}";

} // namespace sdbusplus::bindings::client::${interface.cppNamespacedClass()}

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace sdbusplus::${interface.old_cppNamespacedClass("client")} {

using namespace sdbusplus::bindings::client::${interface.cppNamespacedClass()};

} // namespace sdbusplus::${interface.old_cppNamespacedClass("client")}
#endif
