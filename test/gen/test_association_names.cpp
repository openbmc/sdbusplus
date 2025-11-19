#include "server/Test2/common.hpp"

#include <print>

#include <gtest/gtest.h>

int main()
{
    // If the association is removed from the interface definition, it will
    // cause a build failure in applications still using that association.

    std::println("using association '{}'",
                 sdbusplus::common::server::Test2::association_names::forward::
                     containing);

    EXPECT_EQ(sdbusplus::common::server::Test2::association_names::forward::
                  containing,
              "containing");

    EXPECT_EQ(sdbusplus::common::server::Test2::association_names::reverse::
                  contained_by,
              "contained_by");

    return 0;
}
