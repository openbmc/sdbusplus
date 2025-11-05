#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

#include <chrono>
#include <string>

#include <gtest/gtest.h>

namespace sdbusplus
{
namespace message
{

using namespace std::literals::chrono_literals;

std::string globalId;

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

TEST(CallAsync, Function)
{
    auto b = bus::new_default();
    globalId = "";
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async(setGlobalId);
    b.wait(1s);
    b.process_discard();
    EXPECT_EQ(syncBusId(b), globalId);
}

TEST(CallAsync, FunctionPointer)
{
    auto b = bus::new_default();
    globalId = "";
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async(&setGlobalId);
    b.wait(1s);
    b.process_discard();
    EXPECT_EQ(syncBusId(b), globalId);
}

TEST(CallAsync, Lambda)
{
    auto b = bus::new_default();
    std::string id;
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async([&](message&& m) { m.read(id); });
    b.wait(1s);
    b.process_discard();
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
            b.wait(1s);
            b.process_discard();
        }(),
        "testerror");
}

} // namespace message
} // namespace sdbusplus
