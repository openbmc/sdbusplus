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

message newBusIdReq(bus::bus& b)
{
    return b.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                             "org.freedesktop.DBus", "GetId");
}

std::string syncBusId(bus::bus& b)
{
    std::string ret;
    newBusIdReq(b).call().read(ret);
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
    auto b = bus::new_default();
    int i = 0;
    while (b.process_discard())
        ;
    auto slot = newBusIdReq(b).call_async([&](message&&) {
        ++i;
        throw std::runtime_error("test");
        ++i;
    });
    b.wait(1s);
    b.process_discard();
    EXPECT_EQ(i, 1);
}

} // namespace message
} // namespace sdbusplus
