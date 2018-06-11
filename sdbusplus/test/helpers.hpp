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
 * TODO: Make it support more cases, as I'm sure there are more.
 *
 * @param[in] sdbus_mock - Pointer to your sdbus mock interface used with
 *     the sdbusplus::bus::bus you created.
 * @param[in] defer - Whether object announcement is deferred.
 * @param[in] path - the dbus path passed to the object
 * @param[in] intf - the dbus interface
 * @param[in] properties - an ordered list of expected property updates.
 * @param[in] index - a pointer to a valid integer in a surviving scope.
 */
void SetupDbusObject(sdbusplus::SdBusMock *sdbus_mock, bool defer,
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

    if (properties.empty())
    {
        // We always expect one, but in this case we're not concerned with the
        // output.  If there is more than one property update, we should care
        // and the test writer can add the code to trigger the below case.
        EXPECT_CALL(*sdbus_mock,
                    sd_bus_emit_properties_changed_strv(IsNull(), StrEq(path),
                                                        StrEq(intf), NotNull()))
            .WillOnce(Return(0));
    }
    else
    {
        (*index) = 0;
        EXPECT_CALL(*sdbus_mock,
                    sd_bus_emit_properties_changed_strv(IsNull(), StrEq(path),
                                                        StrEq(intf), NotNull()))
            .Times(properties.size()).WillRepeatedly(Invoke([=](sd_bus *bus,
                                                                const char *path,
                                       const char *interface, char **names) {
                EXPECT_STREQ(properties[(*index)++].c_str(), names[0]);
                return 0;
            }));
    }

    return;
}
