#include <systemd/sd-bus-protocol.h>

#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <chrono>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace sdbusplus
{
namespace message
{

using namespace std::literals::chrono_literals;

std::string globalId;

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

void setGlobalId(message&& m)
{
    m.read(globalId);
}

message newBusIdReq(bus_t& b)
{
    return b.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                             "org.freedesktop.DBus", "GetId");
}

std::string syncBusId(bus_t& b)
{
    auto ret = newBusIdReq(b).call().unpack<std::string>();

    return ret;
}

bool consumeMessagesUntil(bus_t& b, std::function<bool()> done)
{
    auto deadline = std::chrono::steady_clock::now() + 10s;
    while (!done() && std::chrono::steady_clock::now() < deadline)
    {
        b.wait(1s);
        b.process_discard();
    }
    return done();
}

TEST(CallAsync, Function)
{
    auto b = bus::new_default();
    globalId = "";
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async(setGlobalId);
    ASSERT_TRUE(consumeMessagesUntil(b, [&] { return !globalId.empty(); }))
        << "Async callback not invoked within timeout";
    EXPECT_EQ(syncBusId(b), globalId);
}

TEST(CallAsync, FunctionPointer)
{
    auto b = bus::new_default();
    globalId = "";
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async(&setGlobalId);
    ASSERT_TRUE(consumeMessagesUntil(b, [&] { return !globalId.empty(); }))
        << "Async callback not invoked within timeout";
    EXPECT_EQ(syncBusId(b), globalId);
}

TEST(CallAsync, Lambda)
{
    auto b = bus::new_default();
    std::string id;
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async([&](message&& m) { m.read(id); });
    ASSERT_TRUE(consumeMessagesUntil(b, [&] { return !id.empty(); }))
        << "Async callback not invoked within timeout";
    EXPECT_EQ(syncBusId(b), id);
}

TEST(CallAsync, SlotDrop)
{
    auto b = bus::new_default();
    globalId = "";
    while (b.process_discard())
        ;
    {
        auto slot = newBusIdReq(b).call_async(setGlobalId);
    }
    b.wait(1s);
    b.process_discard();
    EXPECT_EQ("", globalId);
}

TEST(CallAsync, ExceptionCaught)
{
    EXPECT_DEATH(
        [] {
            auto b = bus::new_bus();
            while (b.process_discard())
                ;
            auto slot = newBusIdReq(b).call_async([&](message&&) {
                throw std::runtime_error("testerror");
            });
            consumeMessagesUntil(b, [] { return false; });
            // If we get here, the callback never fired
            std::abort();
        }(),
        "testerror");
}

TEST(Call, ThrowsOnSdBusError)
{
    SdBusMock mock;
    message method(nullptr, &mock, std::false_type());

    ON_CALL(mock, sd_bus_error_copy).WillByDefault(testing::Invoke(copyError));
    ON_CALL(mock, sd_bus_error_free).WillByDefault(testing::Invoke(freeError));
    EXPECT_CALL(mock, sd_bus_call(testing::_, testing::_, testing::_,
                                  testing::_, testing::_))
        .WillOnce(testing::Invoke(setNoServiceError));

    EXPECT_THROW(method.call(), sdbusplus::exception::SdBusError);
}

} // namespace message
} // namespace sdbusplus
