#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

#include <gtest/gtest.h>

constexpr auto this_name = "xyz.openbmc_project.sdbusplus.test.Aio";
constexpr auto this_path = "/xyz/openbmc_project/sdbusplus/test/Aio";
constexpr auto this_iface = "xyz.openbmc_project.sdbusplus.test.Aio";
constexpr auto this_property = "ThisProperty";
constexpr auto foo = "Foo";
constexpr auto bar = "Bar";

TEST(AioTest, BasicTest)
{
    boost::asio::io_context io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    EXPECT_NE(nullptr, systemBus);

    systemBus->request_name(this_name);
    sdbusplus::asio::object_server objectServer(systemBus);
}

TEST(AioTest, PropertySetGetTest)
{
    boost::asio::io_context io;
    auto systemBus = std::make_shared<sdbusplus::asio::connection>(io);
    EXPECT_NE(nullptr, systemBus);

    systemBus->request_name(this_name);
    sdbusplus::asio::object_server objectServer(systemBus);
    auto myInterface = objectServer.add_interface(this_path, this_iface);
    myInterface->register_property(this_property, std::string(foo));
    myInterface->initialize();

    // 1. get the property
    auto value = myInterface->get_property<std::string>(this_property);
    EXPECT_NE(value, std::nullopt);
    EXPECT_EQ(*value, foo);

    // 2. bad type
    auto i_val = myInterface->get_property<int>(this_property);
    EXPECT_EQ(i_val, std::nullopt);

    // 3. set and get again
    myInterface->set_property(this_property, std::string{bar});
    auto new_val = myInterface->get_property<std::string>(this_property);
    EXPECT_NE(new_val, std::nullopt);
    EXPECT_EQ(*new_val, bar);

    objectServer.remove_interface(myInterface);
}
