#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <boost/asio.hpp>

int main()
{
    using GetSubTreeType = std::vector<std::pair<
        std::string,
        std::vector<std::pair<std::string, std::vector<std::string>>>>>;
    using message = sdbusplus::message::message;
    // setup connection to dbus
    boost::asio::io_service io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // test async method call and async send
    auto mesg =
        conn->new_method_call("xyz.openbmc_project.ObjectMapper",
                              "/xyz/openbmc_project/object_mapper",
                              "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    static const auto depth = 2;
    static const std::vector<std::string> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    mesg.append("/xyz/openbmc_project/Sensors", depth, interfaces);

    conn->async_send(mesg, [](boost::system::error_code ec, message& ret) {
        std::cout << "async_send callback\n";
        if (ec || ret.is_method_error())
        {
            std::cerr << "error with async_send\n";
            return;
        }
        GetSubTreeType data;
        ret.read(data);
        for (auto& item : data)
        {
            std::cout << item.first << "\n";
        }
    });

    conn->async_method_call(
        [](boost::system::error_code ec, GetSubTreeType& subtree) {
            std::cout << "async_method_call callback\n";
            if (ec)
            {
                std::cerr << "error with async_method_call\n";
                return;
            }
            for (auto& item : subtree)
            {
                std::cout << item.first << "\n";
            }

        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/org/openbmc/control", 2, std::vector<std::string>());

    io.run();

    return 0;
}
