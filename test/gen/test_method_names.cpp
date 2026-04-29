#include "server/TestWithMethod/common.hpp"

#include <cstdio>

#include <gtest/gtest.h>

TEST(MethodNames, TestMethodNames)
{
    // We can access the method name as a symbol.
    // The property name is constexpr.

    constexpr auto methodName =
        sdbusplus::common::server::TestWithMethod::update_value_t::name;

    // The method name can be used as part of error logs.

    printf("error calling method %s\n", methodName);

    // If the method is removed from the interface definition, it will cause a
    // build failure in applications still using that method. That can work
    // even if the application is not (yet) using PDI-generated bindings for
    // it's DBus interactions.

    printf(
        "using method %s \n",
        sdbusplus::common::server::TestWithMethod::update_value_t::name);

    EXPECT_EQ(sdbusplus::common::server::TestWithMethod::update_value_t::name,
              "UpdateValue");
}
