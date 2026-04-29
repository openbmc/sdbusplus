#include "server/Test/common.hpp"
#include "server/Test2/common.hpp"

#include <cstdio>

#include <gtest/gtest.h>

int main()
{
    // We can access the property name as a symbol.
    // The property name is constexpr.

    constexpr auto propName =
        sdbusplus::common::server::Test::some_value_t::name;

    // The property name can be used as part of error logs.

    printf("property %s not found \n", propName);

    // If the property is removed from the interface definition, it will cause a
    // build failure in applications still using that property. That can work
    // even if the application is not (yet) using PDI-generated bindings for
    // it's DBus interactions.

    printf("using property %s \n",
                 sdbusplus::common::server::Test2::new_value_t::name);

    EXPECT_EQ(sdbusplus::common::server::Test2::new_value_t::name, "NewValue");

    return 0;
}
