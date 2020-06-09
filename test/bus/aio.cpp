#include <sdbusplus/bus.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <gtest/gtest.h>

constexpr auto this_name = "xyz.openbmc_project.sdbusplus.test.Aio";



TEST(AioTest, BasicTest)
{
    boost::asio::io_service io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);

    systemBus->request_name(this_name);
    sdbusplus::asio::object_server objectServer(systemBus);

    EXPECT_NE(nullptr, systemBus);
}
