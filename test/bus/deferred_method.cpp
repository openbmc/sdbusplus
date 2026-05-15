#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{

constexpr auto kBusName = "xyz.openbmc_project.sdbusplus.test.DeferredMethod";
constexpr auto kObjectPath = "/xyz/openbmc_project/sdbusplus/test";
constexpr auto kInterfaceName = "xyz.openbmc_project.SdBusPlus.Test";

struct DeferredMethodFixture : public ::testing::Test
{
    boost::asio::io_context io;
    std::shared_ptr<sdbusplus::asio::connection> bus =
        std::make_shared<sdbusplus::asio::connection>(io);
    sdbusplus::asio::object_server server{bus};
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface(kObjectPath, kInterfaceName);
};

TEST_F(DeferredMethodFixture, RegisterBeforeInitializeSucceeds)
{
    bool registered = iface->register_deferred_method<std::string>(
        "Echo", [](sdbusplus::asio::deferred_reply reply,
                   const std::string& input) { reply.send(input); });
    EXPECT_TRUE(registered);
}

TEST_F(DeferredMethodFixture, RegisterVoidReturnSucceeds)
{
    bool registered = iface->register_deferred_method<>(
        "Start", [](sdbusplus::asio::deferred_reply reply) { reply.send(); });
    EXPECT_TRUE(registered);
}

TEST_F(DeferredMethodFixture, RegisterMultipleReturnTypesSucceeds)
{
    bool registered = iface->register_deferred_method<int32_t, std::string>(
        "GetPair", [](sdbusplus::asio::deferred_reply reply) {
            reply.send(42, std::string{"hello"});
        });
    EXPECT_TRUE(registered);
}

TEST_F(DeferredMethodFixture, RegisterErrorReturningHandlerSucceeds)
{
    bool registered = iface->register_deferred_method<std::string>(
        "MayFail", [](sdbusplus::asio::deferred_reply reply,
                      const std::string& /*input*/) {
            // Complete with a custom error built from the request message.
            reply.message().new_method_errno(EIO).method_return();
        });
    EXPECT_TRUE(registered);
}

TEST_F(DeferredMethodFixture, RegisterAfterInitializeFails)
{
    bus->request_name(kBusName);
    ASSERT_TRUE(iface->initialize());

    bool registered = iface->register_deferred_method<>(
        "Late", [](sdbusplus::asio::deferred_reply reply) { reply.send(); });
    EXPECT_FALSE(registered);
}

TEST_F(DeferredMethodFixture, RegisterRejectsInvalidName)
{
    bool registered = iface->register_deferred_method<>(
        "has.dot", [](sdbusplus::asio::deferred_reply reply) { reply.send(); });
    EXPECT_FALSE(registered);
}

TEST(DeferredReply, MoveLeavesSourceInert)
{
    using Reply = sdbusplus::asio::deferred_reply;

    // Construct from an empty message_t; the reply should accept being
    // destroyed without sending (no exception, no UB) because the
    // request_ message is null.
    Reply r{sdbusplus::message_t{nullptr}};
    Reply moved{std::move(r)};
    SUCCEED();
}

} // namespace
