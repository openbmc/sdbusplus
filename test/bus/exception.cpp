#include <systemd/sd-bus-protocol.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <exception>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

int setNoServiceError(sd_bus*, sd_bus_message*, uint64_t, sd_bus_error* error,
                      sd_bus_message**)
{
    return sd_bus_error_set(error, SD_BUS_ERROR_FILE_NOT_FOUND, "No service");
}

int copyError(sd_bus_error* dest, const sd_bus_error* error)
{
    return sd_bus_error_copy(dest, error);
}

void freeError(sd_bus_error* error)
{
    sd_bus_error_free(error);
}

} // namespace

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
        ON_CALL(sdbusMock, sd_bus_error_copy)
            .WillByDefault(testing::Invoke(copyError));
        ON_CALL(sdbusMock, sd_bus_error_free)
            .WillByDefault(testing::Invoke(freeError));
    }

    sdbusplus::message_t newMethod()
    {
        EXPECT_CALL(sdbusMock, sd_bus_message_new_method_call(
                                   testing::_, testing::_, nullptr, nullptr,
                                   nullptr, nullptr))
            .WillOnce(testing::Return(0));
        return bus.new_method_call(nullptr, nullptr, nullptr, nullptr);
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

TEST_F(Exception, BusCallThrowsOnSdBusError)
{
    auto method = newMethod();

    EXPECT_CALL(sdbusMock, sd_bus_call(testing::_, testing::_, testing::_,
                                       testing::_, testing::_))
        .WillOnce(testing::Invoke(setNoServiceError));

    EXPECT_THROW(bus.call(method), sdbusplus::exception::SdBusError);
}

TEST_F(Exception, BusCallNoReplyThrowsOnSdBusError)
{
    auto method = newMethod();

    EXPECT_CALL(sdbusMock, sd_bus_call(testing::_, testing::_, testing::_,
                                       testing::_, testing::_))
        .WillOnce(testing::Invoke(setNoServiceError));

    EXPECT_THROW(bus.call_noreply(method), sdbusplus::exception::SdBusError);
}
