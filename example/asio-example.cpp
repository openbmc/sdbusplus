#include <boost/asio.hpp>
#include <boost/asio/spawn.hpp>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>

int foo(int test)
{
    return ++test;
}

int methodWithMessage(sdbusplus::message::message& m, int test)
{
    return ++test;
}

int voidBar(void)
{
    return 42;
}

#ifdef USE_COROUTINE
void do_start_async_method_call(
    std::shared_ptr<sdbusplus::asio::connection> conn,
    boost::asio::yield_context yield)
{
    boost::system::error_code ec;
    auto testValue = conn->async_method_call<std::tuple<std::string>>(
        yield[ec], "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestMethod", int32_t(42));
    if (!ec && std::get<std::string>(testValue) == "success: 42")
    {
        std::cout << "async_method_call serialized via yield OK!\n";
    }
    else
    {
        std::cout << "ec = " << ec << ": " << std::get<std::string>(testValue)
                  << "\n";
    }
}
#endif

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

    // test object server
    conn->request_name("xyz.openbmc_project.asio-test");
    auto server = sdbusplus::asio::object_server(conn);
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        server.add_interface("/xyz/openbmc_project/test",
                             "xyz.openbmc_project.test");
    // test generic properties
    iface->register_property("int", 33,
                             sdbusplus::asio::PropertyPermission::readWrite);
    std::vector<std::string> myStringVec = {"some", "test", "data"};
    std::vector<std::string> myStringVec2 = {"more", "test", "data"};

    iface->register_property("myStringVec", myStringVec,
                             sdbusplus::asio::PropertyPermission::readWrite);
    iface->register_property("myStringVec2", myStringVec2);

    // test properties with specialized callbacks
    iface->register_property("lessThan50", 23,
                             // custom set
                             [](const int& req, int& propertyValue) {
                                 if (req >= 50)
                                 {
                                     return -EINVAL;
                                 }
                                 propertyValue = req;
                                 return 1; // success
                             });
    iface->register_property(
        "TrailTime", std::string("foo"),
        // custom set
        [](const std::string& req, std::string& propertyValue) {
            propertyValue = req;
            return 1; // success
        },
        // custom get
        [](const std::string& property) {
            auto now = std::chrono::system_clock::now();
            auto timePoint = std::chrono::system_clock::to_time_t(now);
            return property + std::ctime(&timePoint);
        });

    // test method creation
    iface->register_method("TestMethod", [](const int32_t& callCount) {
        return "success: " + std::to_string(callCount);
    });

    iface->register_method("TestFunction", foo);

    iface->register_method("TestMethodWithMessage", methodWithMessage);

    iface->register_method("VoidFunctionReturnsInt", voidBar);

    iface->initialize();
    iface->set_property("int", 45);

    // sd_events work too using the default event loop
    phosphor::Timer t1([]() { std::cerr << "*** tock ***\n"; });
    t1.start(std::chrono::microseconds(1000000));
    phosphor::Timer t2([]() { std::cerr << "*** tick ***\n"; });
    t2.start(std::chrono::microseconds(500000), true);
    // add the sd_event wrapper to the io object
    sdbusplus::asio::sd_event_wrapper sdEvents(io);

#ifdef USE_COROUTINE
    // set up a client to make an async call to the server
    // using coroutines (userspace cooperative multitasking)
    boost::asio::spawn(io, [conn](boost::asio::yield_context yield) {
        do_start_async_method_call(conn, yield);
    });
#endif
    io.run();

    return 0;
}
