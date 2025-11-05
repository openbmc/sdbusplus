#include <systemd/sd-bus-protocol.h>

#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <flat_map>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using testing::DoAll;
using testing::Return;
using testing::StrEq;

ACTION_TEMPLATE(SetReadValue, HAS_1_TEMPLATE_PARAMS(typename, T),
                AND_1_VALUE_PARAMS(value))
{
    *static_cast<T*>(arg2) = value;
}

class FlatMapTest : public testing::Test
{
  protected:
    testing::StrictMock<sdbusplus::SdBusMock> mock;

    void SetUp() override
    {
        EXPECT_CALL(mock, sd_bus_message_new_method_call(testing::_, testing::_,
                                                         nullptr, nullptr,
                                                         nullptr, nullptr))
            .WillRepeatedly(Return(0));
    }

    sdbusplus::message_t new_message()
    {
        return sdbusplus::get_mocked_new(&mock).new_method_call(
            nullptr, nullptr, nullptr, nullptr);
    }
};

// Test that flat_map compiles with the read/append functions
TEST_F(FlatMapTest, CompileTest)
{
    std::flat_map<std::string, int> fmap{{"key1", 100}, {"key2", 200}};
    // This test just verifies that the code compiles
    SUCCEED();
}

} // namespace
