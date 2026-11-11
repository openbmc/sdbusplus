#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message.hpp>

#include <cerrno>
#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

constexpr auto this_name = "xyz.openbmc_project.sdbusplus.test.Aio";

TEST(AioTest, BasicTest)
{
    boost::asio::io_context io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    EXPECT_NE(nullptr, systemBus);

    systemBus->request_name(this_name);
    sdbusplus::asio::object_server objectServer(systemBus);
}

TEST(AioTest, UnpackForwardsMessageOnError)
{
    auto reply =
        sdbusplus::bus::new_default()
            .new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus",
                             "org.freedesktop.DBus", "GetId")
            .call();

    auto err = boost::system::errc::make_error_code(
        boost::system::errc::permission_denied);

    bool gotMsg = false;
    sdbusplus::asio::connection::unpack(
        err, reply,
        [&](boost::system::error_code ec, sdbusplus::message_t msg) {
            EXPECT_TRUE(ec);
            gotMsg = msg.get_cookie() != 0;
        });
    EXPECT_TRUE(gotMsg) << "Message should be available in every handler call";
}

class YieldMethodCallTest : public ::testing::Test
{
  protected:
    static constexpr auto path = "/xyz/openbmc_project/sdbusplus/test/aio";
    static constexpr auto interface = "xyz.openbmc_project.sdbusplus.test.Aio";

    YieldMethodCallTest() :
        conn(std::make_shared<sdbusplus::asio::connection>(io)), server(conn)
    {
        conn->request_name(this_name);
        iface = server.add_interface(path, interface);
        iface->register_method(
            "Fail", [](boost::asio::yield_context) -> int32_t {
                throw sdbusplus::exception::SdBusError(ETIMEDOUT, "test");
            });
        iface->register_method(
            "FailString", [](boost::asio::yield_context) -> std::string {
                throw sdbusplus::exception::SdBusError(ETIMEDOUT, "test");
            });
        iface->register_method("Get", []() -> int32_t { return 42; });
        iface->initialize();
    }

    template <typename Fn>
    void run(Fn&& fn)
    {
        boost::asio::spawn(
            io,
            [this, &fn](boost::asio::yield_context yield) {
                fn(yield);
                io.stop();
            },
            boost::asio::detached);
        io.run();
    }

    boost::asio::io_context io;
    std::shared_ptr<sdbusplus::asio::connection> conn;
    sdbusplus::asio::object_server server;
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface;
};

TEST_F(YieldMethodCallTest, ErrorReplyKeepsErrorCode)
{
    run([this](boost::asio::yield_context yield) {
        boost::system::error_code ec;
        auto value = conn->yield_method_call<int32_t>(yield, ec, this_name,
                                                      path, interface, "Fail");
        EXPECT_EQ(boost::system::errc::timed_out, ec);
        EXPECT_EQ(0, value);
    });
}

TEST_F(YieldMethodCallTest, ErrorReplyReturnsEmptyString)
{
    run([this](boost::asio::yield_context yield) {
        boost::system::error_code ec;
        auto value = conn->yield_method_call<std::string>(
            yield, ec, this_name, path, interface, "FailString");
        EXPECT_EQ(boost::system::errc::timed_out, ec);
        EXPECT_EQ("", value);
    });
}

TEST_F(YieldMethodCallTest, SuccessReplyReturnsValue)
{
    run([this](boost::asio::yield_context yield) {
        boost::system::error_code ec;
        auto value = conn->yield_method_call<int32_t>(yield, ec, this_name,
                                                      path, interface, "Get");
        EXPECT_FALSE(ec);
        EXPECT_EQ(42, value);
    });
}

TEST_F(YieldMethodCallTest, SuccessReplyWithWrongTypeIsInvalidArgument)
{
    run([this](boost::asio::yield_context yield) {
        boost::system::error_code ec;
        auto value = conn->yield_method_call<std::string>(
            yield, ec, this_name, path, interface, "Get");
        EXPECT_EQ(boost::system::errc::invalid_argument, ec);
        EXPECT_EQ("", value);
    });
}
