#include <boost/asio/io_context.hpp>
#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

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
