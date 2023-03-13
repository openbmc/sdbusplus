#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <exception>

#include <gtest/gtest.h>

class Exception : public ::testing::Test
{
  protected:
    sdbusplus::SdBusMock sdbusMock;
    sdbusplus::bus_t bus = sdbusplus::get_mocked_new(&sdbusMock);
    std::exception_ptr e =
        std::make_exception_ptr(std::runtime_error{"current exception"});

    void SetUp() override
    {
        bus.set_current_exception(e);
    }
};

TEST_F(Exception, BusProcessRethrowsTheCurrentException)
{
    EXPECT_THROW(bus.process(), std::runtime_error);
}

TEST_F(Exception, BusProcessDiscardRethrowsTheCurrentException)
{
    EXPECT_THROW(bus.process_discard(), std::runtime_error);
}
