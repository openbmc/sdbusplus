#include "server/Test/common.hpp"
#include "server/Test2/common.hpp"

#include <print>

#include <gtest/gtest.h>

int main()
{
    // We can access the property name as a symbol.
    // The property name is constexpr.

    constexpr auto propName =
        sdbusplus::common::server::Test::property_names::someValue;

    // The property name can be used as part of error logs.

    std::println("property {} not found \n", propName);

    // If the property is removed from the interface definition, it will cause a
    // build failure in applications still using that property. That can work
    // even if the application is not (yet) using PDI-generated bindings for
    // it's DBus interactions.

    std::println("using property {} \n",
                 sdbusplus::common::server::Test2::property_names::newValue);

    EXPECT_EQ(sdbusplus::common::server::Test2::property_names::newValue,
              "NewValue");

    return 0;
}
