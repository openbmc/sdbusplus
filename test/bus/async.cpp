#include <chrono>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/common_calls/properties.hpp>
#include <sdbusplus/typed.hpp>
#include <sdeventplus/event.hpp>
#include <sdeventplus/utility/future.hpp>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(AsyncCall, BasicAsync)
{
    auto bus = sdbusplus::bus::new_bus();
    auto req =
        bus.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                            "org.freedesktop.DBus", "GetId");
    size_t called = 0;
    auto cb = [&](sdbusplus::message::message reply) {
        std::string id;
        reply.read(id);
        fprintf(stderr, "Got ID: %s\n", id.c_str());
        called++;
    };
    bus.call_async(req, std::move(cb)).set_floating(true);
    EXPECT_EQ(0, called);
    fprintf(stderr, "Running Async Loop\n");
    while (bus.wait(10s))
    {
        while (bus.process_discard())
        {
        }
        if (called > 0)
        {
            break;
        }
    }
    fprintf(stderr, "Done Async Loop\n");
    EXPECT_EQ(1, called);
}

TEST(AsyncCall, Future)
{
    auto bus = sdbusplus::bus::new_bus();
    auto req =
        bus.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                            "org.freedesktop.DBus", "GetId");
    auto reply = bus.call_async_future(req);
    fprintf(stderr, "Running Async Loop\n");
    while (bus.wait(10s))
    {
        while (bus.process_discard())
        {
        }
        if (reply.is_ready())
        {
            break;
        }
    }
    fprintf(stderr, "Done Async Loop\n");
    std::string id;
    reply.get().read(id);
    fprintf(stderr, "Got ID: %s\n", id.c_str());
}

TEST(AsyncCall, MultiFuture)
{
    auto event = sdeventplus::Event::get_new();
    auto bus = sdbusplus::bus::new_bus();
    bus.attach_event(event.get(), 0);
    std::vector<stdplus::util::Future<sdbusplus::message::message>> v;
    for (size_t i = 0; i < 100; ++i)
    {
        auto req =
            bus.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetId");
        v.push_back(bus.call_async_future(req));
    }
    auto cb = [&](const sdeventplus::Event& event, decltype(v)&& v) {
        for (auto& m : v)
        {
            std::string id;
            std::move(m.get()).read(id);
            fprintf(stderr, "Msg: %s\n", id.c_str());
        }
        event.exit(0);
    };
    sdeventplus::utility::callWhenReady(event, std::move(cb), std::move(v))
        .set_floating(true);
    EXPECT_EQ(0, event.loop());
}

TEST(AsyncCall, MultiSync)
{
    auto bus = sdbusplus::bus::new_bus();
    for (size_t i = 0; i < 100; ++i)
    {
        auto req =
            bus.new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                                "org.freedesktop.DBus", "GetId");
        auto reply = bus.call(req);
        std::string id;
        reply.read(id);
        fprintf(stderr, "Msg: %s\n", id.c_str());
    }
}

auto callNameHasOwner(sdbusplus::bus::bus& bus,
                      const char* service = "org.freedesktop.DBus",
                      const char* obj = "/org/freedesktop/DBus")
{
    using Req = sdbusplus::typed::Types<std::string>;
    using Ret = sdbusplus::typed::Types<bool>;
    return sdbusplus::typed::MethodCall<Req, Ret>(bus.new_method_call(
        service, obj, "org.freedesktop.DBus", "NameHasOwner"));
}

TEST(MethodCall, Call)
{
    auto bus = sdbusplus::bus::new_bus();
    bool ret;
    callNameHasOwner(bus).append("org.freedesktop.DBus").call().read(ret);
    fprintf(stderr, "Got value: %d\n", ret);
    callNameHasOwner(bus).append("Hi").call().read(ret);
    fprintf(stderr, "Got value: %d\n", ret);
}

TEST(MethodCall, GetProperty)
{
    auto bus = sdbusplus::bus::new_bus();
    auto features =
        sdbusplus::common_calls::properties::get<std::vector<std::string>>(
            bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus", "Interfaces");
    for (const auto& feature : features)
    {
        fprintf(stderr, "Got feature: %s\n", feature.c_str());
    }
}

TEST(MethodCall, GetProperties)
{
    auto bus = sdbusplus::bus::new_bus();
    auto features =
        sdbusplus::common_calls::properties::getAll<std::vector<std::string>>(
            bus, "org.freedesktop.DBus", "/org/freedesktop/DBus",
            "org.freedesktop.DBus");
    for (const auto& [name, values] : features)
    {
        for (const auto& value : std::get<std::vector<std::string>>(values))
        {
            fprintf(stderr, "Got value: %s %s\n", name.c_str(), value.c_str());
        }
    }
}

TEST(MethodCall, CallAsync)
{
    auto bus = sdbusplus::bus::new_bus();
    auto reply = callNameHasOwner(bus)
                     .append("org.freedesktop.DBus")
                     .call_async_future();
    while (bus.wait(10s))
    {
        while (bus.process_discard())
        {
        }
        if (reply.is_ready())
        {
            break;
        }
    }
    fprintf(stderr, "Done async\n");
    bool ret;
    std::move(reply.get()).read(ret);
    fprintf(stderr, "Got value: %d\n", ret);
}
