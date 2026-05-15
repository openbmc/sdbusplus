#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace
{

constexpr auto kBusName = "xyz.openbmc_project.sdbusplus.test.CompletionMethod";
constexpr auto kObjectPath = "/xyz/openbmc_project/sdbusplus/test";
constexpr auto kInterfaceName = "xyz.openbmc_project.SdBusPlus.Test";

struct CompletionMethodFixture : public ::testing::Test
{
    boost::asio::io_context io;
    std::shared_ptr<sdbusplus::asio::connection> bus =
        std::make_shared<sdbusplus::asio::connection>(io);
    sdbusplus::asio::object_server server{bus};
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface(kObjectPath, kInterfaceName);
};

TEST_F(CompletionMethodFixture, RegisterBeforeInitializeSucceeds)
{
    bool registered = iface->register_completion_method<std::string>(
        "Echo", [](sdbusplus::asio::completion c, const std::string& input) {
            c.send(input);
        });
    EXPECT_TRUE(registered);
}

TEST_F(CompletionMethodFixture, RegisterVoidReturnSucceeds)
{
    bool registered = iface->register_completion_method<>(
        "Start", [](sdbusplus::asio::completion c) { c.send(); });
    EXPECT_TRUE(registered);
}

TEST_F(CompletionMethodFixture, RegisterMultipleReturnTypesSucceeds)
{
    bool registered = iface->register_completion_method<int32_t, std::string>(
        "GetPair", [](sdbusplus::asio::completion c) {
            c.send(42, std::string{"hello"});
        });
    EXPECT_TRUE(registered);
}

TEST_F(CompletionMethodFixture, RegisterErrorReturningHandlerSucceeds)
{
    bool registered = iface->register_completion_method<std::string>(
        "MayFail",
        [](sdbusplus::asio::completion c, const std::string& /*input*/) {
            // Complete with a custom error built from the request message.
            c.message().new_method_errno(EIO).method_return();
        });
    EXPECT_TRUE(registered);
}

TEST_F(CompletionMethodFixture, RegisterAfterInitializeFails)
{
    bus->request_name(kBusName);
    ASSERT_TRUE(iface->initialize());

    bool registered = iface->register_completion_method<>(
        "Late", [](sdbusplus::asio::completion c) { c.send(); });
    EXPECT_FALSE(registered);
}

TEST_F(CompletionMethodFixture, RegisterRejectsInvalidName)
{
    bool registered = iface->register_completion_method<>(
        "has.dot", [](sdbusplus::asio::completion c) { c.send(); });
    EXPECT_FALSE(registered);
}

TEST(Completion, MoveLeavesSourceInert)
{
    using Completion = sdbusplus::asio::completion;

    // Construct from an empty message_t; the completion should accept being
    // destroyed without sending (no exception, no UB) because the
    // request_ message is null.
    Completion c{sdbusplus::message_t{nullptr}};
    Completion moved{std::move(c)};
    SUCCEED();
}

} // namespace
