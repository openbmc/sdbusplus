#include <gtest/gtest.h>
#include <sdbusplus/bus.hpp>

constexpr auto this_name = "xyz.openbmc_project.sdbusplus.test.ListNames";


TEST(ListNames, NoServiceName)
{
    auto b = sdbusplus::bus::new_default();
    auto names = b.list_names_acquired();

    EXPECT_EQ(names.cend(), std::find(names.cbegin(), names.cend(), this_name));
}

TEST(ListNames, ClaimServiceName)
{
    auto b = sdbusplus::bus::new_default();
    b.request_name(this_name);
    auto names = b.list_names_acquired();

    auto i = std::find(names.cbegin(), names.cend(), this_name);

    ASSERT_NE(names.cend(), i);
    EXPECT_EQ(this_name, *i);
}

TEST(ListNames, FindDbusServer)
{
    auto b = sdbusplus::bus::new_default();
    auto names = b.list_names_acquired();

    auto dbus_server = "org.freedesktop.DBus";
    auto i = std::find(names.cbegin(), names.cend(), dbus_server);

    ASSERT_NE(names.cend(), i);
    EXPECT_EQ(dbus_server, *i);
}
