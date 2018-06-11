#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sdbusplus/test/sdbus_mock.hpp>
#include <string>
#include <vector>

using ::testing::Invoke;
using ::testing::IsNull;
using ::testing::NotNull;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::_;

/** @brief Setup the expectations for sdbus-based object creation.
 *
 * Objects created that inherit a composition from sdbusplus will all
 * require at least these expectations.
 *
 * If you have future sd_bus_emit_properties_changed_strv calls expected,
 * you'll need to add those calls into your test.  This only captures the
 * property updates you tell it to expect initially.
 *
 * TODO: Make it support more cases, as I'm sure there are more.
 *
 * @param[in] sdbus_mock - Pointer to your sdbus mock interface used with
 *     the sdbusplus::bus::bus you created.
 * @param[in] defer - Whether object announcement is deferred.
 * @param[in] path - the dbus path passed to the object
 * @param[in] intf - the dbus interface
 * @param[in] properties - an ordered list of expected property updates.
 * @param[in] index - a pointer to a valid integer in a surviving scope.
 *
 * @example:
 * The following example doesn't check for any property emit calls:
 * (example taken from phosphor-pid-control: pid_zone_unittest.cpp
 *
 * sdbusplus::SdBusMock sdbus_mock_passive, sdbus_mock_host, sdbus_mock_mode;
 * auto bus_mock_passive = sdbusplus::get_mocked_new(&sdbus_mock_passive);
 * auto bus_mock_host = sdbusplus::get_mocked_new(&sdbus_mock_host);
 * auto bus_mock_mode = sdbusplus::get_mocked_new(&sdbus_mock_mode);
 *
 * EXPECT_CALL(sdbus_mock_host,
 *             sd_bus_add_object_manager(
 *                 IsNull(),
 *                 _,
 *                 StrEq("/xyz/openbmc_project/extsensors")))
 *     .WillOnce(Return(0));
 *
 *  SensorManager m(std::move(bus_mock_passive),
 *                  std::move(bus_mock_host));
 *
 * bool defer = true;
 * const char *objPath = "/path/";
 * int64_t zone = 1;
 * float minThermalRpm = 1000.0;
 * float failSafePercent = 0.75;
 *
 * int i;
 * std::vector<std::string> properties;
 * ExpectDbusObject(&sdbus_mock_mode, defer, objPath, modeInterface,
 *                  properties, &i);
 *
 * PIDZone p(zone, minThermalRpm, failSafePercent, m, bus_mock_mode, objPath,
 *           defer);
 *
 * @example:
 * The following example checks that the Target property was emitted.
 * (example taken from phosphor-hwmon: fanpwm_unittest.cpp
 *
 * int i;
 * std::vector<std::string> properties = {"Target"};
 * sdbusplus::SdBusMock sdbus_mock;
 * auto bus_mock = sdbusplus::get_mocked_new(&sdbus_mock);
 * std::string instancePath = "";
 * std::string devPath = "devp";
 * std::string id = "the_id";
 * std::string objPath = "asdf";
 * bool defer = true;
 * uint64_t target = 0x01;
 *
 * std::unique_ptr<hwmonio::HwmonIOInterface> hwmonio_mock =
 *     std::make_unique<hwmonio::HwmonIOMock>();
 *
 * ExpectDbusObject(&sdbus_mock, defer, objPath, FanPwmIntf, properties, &i);
 *
 * hwmonio::HwmonIOMock *hwmonio =
 *     reinterpret_cast<hwmonio::HwmonIOMock *>(hwmonio_mock.get());
 *
 * hwmon::FanPwm f(std::move(hwmonio_mock),
 *                  devPath,
 *                  id,
 *                  bus_mock,
 *                  objPath.c_str(),
 *                  defer,
 *                  target);
 */
void ExpectDbusObject(sdbusplus::SdBusMock *sdbus_mock, bool defer,
                      const std::string &path, const std::string &intf,
                      const std::vector<std::string> &properties, int *index)
{
    EXPECT_CALL(*sdbus_mock,
                sd_bus_add_object_vtable(IsNull(), NotNull(), StrEq(path),
                                         StrEq(intf), NotNull(), NotNull()))
        .WillOnce(Return(0));

    if (!defer)
    {
        EXPECT_CALL(*sdbus_mock,
                    sd_bus_emit_object_added(IsNull(), StrEq(path)))
            .WillOnce(Return(0));
    }

    if (!properties.empty())
    {
        (*index) = 0;
        EXPECT_CALL(*sdbus_mock,
                    sd_bus_emit_properties_changed_strv(IsNull(), StrEq(path),
                                                        StrEq(intf), NotNull()))
            .Times(properties.size())
            .WillRepeatedly(Invoke([=](sd_bus *bus, const char *path,
                                       const char *interface, char **names) {
                EXPECT_STREQ(properties[(*index)++].c_str(), names[0]);
                return 0;
            }));
    }

    return;
}
