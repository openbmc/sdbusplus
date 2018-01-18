#include <gtest/gtest.h>
#include <sdbusplus/bus.hpp>

constexpr auto this_name = "xyz.openbmc_project.sdbusplus.test.ListNames";

class ListNames : public ::testing::Test
{
  protected:
    decltype(sdbusplus::bus::new_default()) bus = sdbusplus::bus::new_default();
};

TEST_F(ListNames, NoServiceNameWithoutRequestName)
{
    auto names = bus.list_names_acquired();

    EXPECT_EQ(names.cend(), std::find(names.cbegin(), names.cend(), this_name));
}

TEST_F(ListNames, HasServiceNameAfterRequestName)
{
    bus.request_name(this_name);
    auto names = bus.list_names_acquired();

    auto i = std::find(names.cbegin(), names.cend(), this_name);

    ASSERT_NE(names.cend(), i);
    EXPECT_EQ(this_name, *i);
}

TEST_F(ListNames, HasUniqueName)
{
    auto names = bus.list_names_acquired();

    ASSERT_FALSE(bus.get_unique_name().empty());
    EXPECT_NE(names.cend(),
              std::find(names.cbegin(), names.cend(), bus.get_unique_name()));
}

TEST_F(ListNames, HasDbusServer)
{
    auto names = bus.list_names_acquired();

    auto dbus_server = "org.freedesktop.DBus";
    auto i = std::find(names.cbegin(), names.cend(), dbus_server);

    ASSERT_NE(names.cend(), i);
    EXPECT_EQ(dbus_server, *i);
}
