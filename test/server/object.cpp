#include "Test/server.hpp"

#include <sdbusplus/bus.hpp>
#include <sdbusplus/server/manager.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <gtest/gtest.h>

using ::testing::_;
using ::testing::StrEq;

using TestInherit =
    sdbusplus::server::object::object<sdbusplus::server::server::Test>;

class Object : public ::testing::Test
{
  protected:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus::bus bus = sdbusplus::get_mocked_new(&sdbusMock);

    static constexpr auto busName = "xyz.openbmc_project.sdbusplus.test.Object";
    static constexpr auto objPath = "/xyz/openbmc_project/sdbusplus/test";
};

TEST_F(Object, InterfaceAddedOnly)
{
    // Simulate the typical usage of a service
    sdbusplus::server::manager::manager objManager(bus, objPath);
    bus.request_name(busName);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(1);

    // This only add interface to the existing object
    auto test = std::make_unique<TestInherit>(
        bus, objPath, TestInherit::action::emit_interface_added);

    // After destruction, the interface shall be removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(1);
}

TEST_F(Object, DeferAddInterface)
{
    // Simulate the typical usage of a service
    sdbusplus::server::manager::manager objManager(bus, objPath);
    bus.request_name(busName);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(0);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(0);

    // It defers emit_object_added()
    auto test = std::make_unique<TestInherit>(bus, objPath,
                                              TestInherit::action::defer_emit);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(0);

    // Now invoke emit_object_added()
    test->emit_object_added();

    // After destruction, the object will be removed
    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(0);
}

TEST_F(Object, ObjectAdded)
{
    // Simulate the typical usage of a service
    sdbusplus::server::manager::manager objManager(bus, objPath);
    bus.request_name(busName);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_added(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_added_strv(_, StrEq(objPath), _))
        .Times(0);

    // Note: this is the wrong usage!
    // With the below code, the interface is added as expected, but after
    // destruction of TestInherit, the whole object will be removed from DBus.
    // Refer to InterfaceAddedOnly case for the correct usage.
    auto test = std::make_unique<TestInherit>(bus, objPath);

    EXPECT_CALL(sdbusMock, sd_bus_emit_object_removed(_, StrEq(objPath)))
        .Times(1);
    EXPECT_CALL(sdbusMock,
                sd_bus_emit_interfaces_removed_strv(_, StrEq(objPath), _))
        .Times(0);
}
