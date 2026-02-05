#include "full_tree.hpp"
#include "sdbusplus/asio/connection.hpp"
#include "server/Test/Nested/common.hpp"
#include "server/Test/common.hpp"
#include "server/Test2/common.hpp"

#include <print>

#include <gtest/gtest.h>

// Test for the full tree header
TEST(FullTreeTest, InterfaceAccess)
{
    std::println("using property {} \n",
                 decltype(server.Test)::property_names::some_value);

    // CONCERN: being able to grep for the interface name
    // depends on clang-format not breaking up the member access along the '.'
    //
    // Currently assuming it should be possible to teach clang-format to avoid
    // splitting it.

    // below means we can write e.g.
    //
    //    xyz.openbmc_project.Inventory.Item.interface
    //
    // and it should resolve to the string literal (interface name)
    std::println("using interface {} \n", server.Test.interface);

    std::println("using interface {} \n", server.Test.Nested.interface);

    std::println("using signal {} \n",
                 decltype(server.Test2)::signal_names::other_value_changed);
}
