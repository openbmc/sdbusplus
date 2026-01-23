#include "server/Test/common.hpp"

#include <print>

#include <gtest/gtest.h>

using Test = sdbusplus::common::server::Test;

int main()
{
    // can be used as part of error messages to support debug/developer
    // workflows

    const std::string valueReceived = Test::enum_strings::EnumOne::one_b;

    std::println("expected enum value {}, but got {} \n",
                 Test::enum_strings::EnumOne::one_a, valueReceived);

    // If the enum member is removed from the interface definition, it will
    // cause a build failure in applications still using that enum member. That
    // can work even if the application is not (yet) using PDI-generated
    // bindings for it's DBus interactions.

    std::println("using enum {} \n", Test::enum_strings::EnumOne::one_a);

    // confirm the literal value
    EXPECT_EQ(Test::enum_strings::EnumOne::one_a, "server.Test.EnumOne.OneA");
    EXPECT_EQ(Test::enum_strings::EnumOne::one_b, "server.Test.EnumOne.OneB");

    // confirm it is generated for multiple enums
    EXPECT_EQ(Test::enum_strings::EnumTwo::two_a, "server.Test.EnumTwo.TwoA");
    EXPECT_EQ(Test::enum_strings::EnumTwo::two_b, "server.Test.EnumTwo.TwoB");

    return 0;
}
