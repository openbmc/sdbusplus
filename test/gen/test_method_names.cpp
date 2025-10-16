#include "server/TestWithMethod/common.hpp"

#include <print>

#include <gtest/gtest.h>

TEST(MethodNames, TestMethodNames)
{
    // We can access the method name as a symbol.
    // The property name is constexpr.

    constexpr auto methodName =
        sdbusplus::common::server::TestWithMethod::method_names::update_value;

    // The method name can be used as part of error logs.

    std::println("error calling method {}\n", methodName);

    // If the method is removed from the interface definition, it will cause a
    // build failure in applications still using that method. That can work
    // even if the application is not (yet) using PDI-generated bindings for
    // it's DBus interactions.

    std::println(
        "using method {} \n",
        sdbusplus::common::server::TestWithMethod::method_names::update_value);

    EXPECT_EQ(
        sdbusplus::common::server::TestWithMethod::method_names::update_value,
        "UpdateValue");
}
