#pragma once

#include <${interface.headerFile("common")}>

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace sdbusplus::${interface.old_cppNamespacedClass("client")} {

constexpr auto interface =
    sdbusplus::bindings::common::${interface.cppNamespacedClass()}::interface;

} // namespace sdbusplus::${interface.old_cppNamespacedClass("client")}
#endif

namespace sdbusplus::bindings::client::${interface.cppNamespacedClass()}
{

} // namespace sdbusplus::bindings::client::${interface.cppNamespacedClass()}
