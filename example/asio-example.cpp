#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/spawn.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/sd_event.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/server.hpp>
#include <sdbusplus/timer.hpp>

#include <chrono>
#include <ctime>
#include <iostream>
#include <variant>

using variant = std::variant<int, std::string>;

int foo(int test)
{
    std::cout << "foo(" << test << ") -> " << (test + 1) << "\n";
    return ++test;
}

// called from coroutine context, can make yielding dbus calls
int fooYield(boost::asio::yield_context yield,
             std::shared_ptr<sdbusplus::asio::connection> conn, int test)
{
    // fetch the real value from testFunction
    boost::system::error_code ec;
    std::cout << "fooYield(yield, " << test << ")...\n";
    int testCount = conn->yield_method_call<int>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestFunction", test);
    if (ec || testCount != (test + 1))
    {
        std::cout << "call to foo failed: ec = " << ec << '\n';
        return -1;
    }
    std::cout << "yielding call to foo OK! (-> " << testCount << ")\n";
    return testCount;
}

int methodWithMessage(sdbusplus::message_t& /*m*/, int test)
{
    std::cout << "methodWithMessage(m, " << test << ") -> " << (test + 1)
              << "\n";
    return ++test;
}

int voidBar(void)
{
    std::cout << "voidBar() -> 42\n";
    return 42;
}

void do_start_async_method_call_one(
    std::shared_ptr<sdbusplus::asio::connection> conn,
    boost::asio::yield_context yield)
{
    boost::system::error_code ec;
    variant testValue;
    conn->yield_method_call<>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "org.freedesktop.DBus.Properties", "Set", "xyz.openbmc_project.test",
        "int", variant(24));
    testValue = conn->yield_method_call<variant>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "org.freedesktop.DBus.Properties", "Get", "xyz.openbmc_project.test",
        "int");
    if (!ec && std::get<int>(testValue) == 24)
    {
        std::cout << "async call to Properties.Get serialized via yield OK!\n";
    }
    else
    {
        std::cout << "ec = " << ec << ": " << std::get<int>(testValue) << "\n";
    }
    conn->yield_method_call<void>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "org.freedesktop.DBus.Properties", "Set", "xyz.openbmc_project.test",
        "int", variant(42));
    testValue = conn->yield_method_call<variant>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "org.freedesktop.DBus.Properties", "Get", "xyz.openbmc_project.test",
        "int");
    if (!ec && std::get<int>(testValue) == 42)
    {
        std::cout << "async call to Properties.Get serialized via yield OK!\n";
    }
    else
    {
        std::cout << "ec = " << ec << ": " << std::get<int>(testValue) << "\n";
    }
}

void do_start_async_ipmi_call(std::shared_ptr<sdbusplus::asio::connection> conn,
                              boost::asio::yield_context yield)
{
    auto method = conn->new_method_call("xyz.openbmc_project.asio-test",
                                        "/xyz/openbmc_project/test",
                                        "xyz.openbmc_project.test", "execute");
    constexpr uint8_t netFn = 6;
    constexpr uint8_t lun = 0;
    constexpr uint8_t cmd = 1;
    std::map<std::string, variant> options = {{"username", variant("admin")},
                                              {"privilege", variant(4)}};
    std::vector<uint8_t> commandData = {4, 3, 2, 1};
    method.append(netFn, lun, cmd, commandData, options);
    boost::system::error_code ec;
    sdbusplus::message_t reply = conn->async_send_yield(method, yield[ec]);
    std::tuple<uint8_t, uint8_t, uint8_t, uint8_t, std::vector<uint8_t>>
        tupleOut;
    try
    {
        reply.read(tupleOut);
    }
    catch (const sdbusplus::exception::exception& e)
    {
        std::cerr << "failed to unpack; sig is " << reply.get_signature()
                  << "\n";
    }
    auto& [rnetFn, rlun, rcmd, cc, responseData] = tupleOut;
    std::vector<uint8_t> expRsp = {1, 2, 3, 4};
    if (rnetFn == uint8_t(netFn + 1) && rlun == lun && rcmd == cmd && cc == 0 &&
        responseData == expRsp)
    {
        std::cerr << "ipmi call returns OK!\n";
    }
    else
    {
        std::cerr << "ipmi call returns unexpected response\n";
    }
}

auto ipmiInterface(boost::asio::yield_context /*yield*/, uint8_t netFn,
                   uint8_t lun, uint8_t cmd, std::vector<uint8_t>& /*data*/,
                   const std::map<std::string, variant>& /*options*/)
{
    std::vector<uint8_t> reply = {1, 2, 3, 4};
    uint8_t cc = 0;
    std::cerr << "ipmiInterface:execute(" << int(netFn) << int(cmd) << ")\n";
    return std::make_tuple(uint8_t(netFn + 1), lun, cmd, cc, reply);
}

void do_start_async_to_yield(std::shared_ptr<sdbusplus::asio::connection> conn,
                             boost::asio::yield_context yield)
{
    boost::system::error_code ec;
    int testValue = 0;
    testValue = conn->yield_method_call<int>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestYieldFunction", int(41));

    if (!ec && testValue == 42)
    {
        std::cout
            << "yielding call to TestYieldFunction serialized via yield OK!\n";
    }
    else
    {
        std::cout << "ec = " << ec << ": " << testValue << "\n";
    }

    ec.clear();
    auto badValue = conn->yield_method_call<std::string>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestYieldFunction", int(41));

    if (!ec)
    {
        std::cout
            << "yielding call to TestYieldFunction returned the wrong type\n";
    }
    else
    {
        std::cout << "TestYieldFunction expected error: " << ec << "\n";
    }

    ec.clear();
    auto unUsedValue = conn->yield_method_call<std::string>(
        yield, ec, "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestYieldFunctionNotExits", int(41));

    if (!ec)
    {
        std::cout << "TestYieldFunctionNotExists returned unexpectedly\n";
    }
    else
    {
        std::cout << "TestYieldFunctionNotExits expected error: " << ec << "\n";
    }
}

int server()
{
    // setup connection to dbus
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

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
                                     return false;
                                 }
                                 propertyValue = req;
                                 return true;
                             });
    iface->register_property(
        "TrailTime", std::string("foo"),
        // custom set
        [](const std::string& req, std::string& propertyValue) {
            propertyValue = req;
            return true;
        },
        // custom get
        [](const std::string& property) {
            auto now = std::chrono::system_clock::now();
            auto timePoint = std::chrono::system_clock::to_time_t(now);
            return property + std::ctime(&timePoint);
        });

    // test method creation
    iface->register_method("TestMethod", [](const int32_t& callCount) {
        return std::make_tuple(callCount,
                               "success: " + std::to_string(callCount));
    });

    iface->register_method("TestFunction", foo);

    // fooYield has boost::asio::yield_context as first argument
    // so will be executed in coroutine context if called
    iface->register_method("TestYieldFunction",
                           [conn](boost::asio::yield_context yield, int val) {
                               return fooYield(yield, conn, val);
                           });

    iface->register_method("TestMethodWithMessage", methodWithMessage);

    iface->register_method("VoidFunctionReturnsInt", voidBar);

    iface->register_method("execute", ipmiInterface);

    iface->initialize();

    io.run();

    return 0;
}

int client()
{
    using GetSubTreeType = std::vector<std::pair<
        std::string,
        std::vector<std::pair<std::string, std::vector<std::string>>>>>;
    using message = sdbusplus::message_t;

    // setup connection to dbus
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    int ready = 0;
    while (!ready)
    {
        auto readyMsg = conn->new_method_call(
            "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
            "xyz.openbmc_project.test", "VoidFunctionReturnsInt");
        try
        {
            message intMsg = conn->call(readyMsg);
            intMsg.read(ready);
        }
        catch (const sdbusplus::exception::exception& e)
        {
            ready = 0;
            // pause to give the server a chance to start up
            usleep(10000);
        }
    }

    // test async method call and async send
    auto mesg =
        conn->new_method_call("xyz.openbmc_project.ObjectMapper",
                              "/xyz/openbmc_project/object_mapper",
                              "xyz.openbmc_project.ObjectMapper", "GetSubTree");

    int32_t depth = 2;
    constexpr std::array<std::string_view, 1> interfaces{
        "xyz.openbmc_project.Sensor.Value"};
    mesg.append("/xyz/openbmc_project/Sensors", depth, interfaces);

    conn->async_send(mesg, [](boost::system::error_code ec, message& ret) {
        std::cout << "async_send callback\n";
        if (ec || ret.is_method_error())
        {
            std::cerr << "error with async_send\n";
            return;
        }
        auto data = ret.unpack<GetSubTreeType>();

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

    std::string nonConstCapture = "lalalala";
    conn->async_method_call(
        [nonConstCapture = std::move(nonConstCapture)](
            boost::system::error_code ec,
            const std::vector<std::string>& /*things*/) mutable {
            std::cout << "async_method_call callback\n";
            nonConstCapture += " stuff";
            if (ec)
            {
                std::cerr << "async_method_call expected failure: " << ec
                          << "\n";
            }
            else
            {
                std::cerr << "async_method_call should have failed!\n";
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", depth, interfaces);

    // sd_events work too using the default event loop
    sdbusplus::Timer t1([]() { std::cerr << "*** tock ***\n"; });
    t1.start(std::chrono::microseconds(1000000));
    sdbusplus::Timer t2([]() { std::cerr << "*** tick ***\n"; });
    t2.start(std::chrono::microseconds(500000), true);
    // add the sd_event wrapper to the io object
    sdbusplus::asio::sd_event_wrapper sdEvents(io);

    // set up a client to make an async call to the server
    // using coroutines (userspace cooperative multitasking)
    (void)boost::asio::spawn(
        io,
        [conn](boost::asio::yield_context yield) {
            do_start_async_method_call_one(conn, yield);
        },
        boost::asio::detached);
    (void)boost::asio::spawn(
        io,
        [conn](boost::asio::yield_context yield) {
            do_start_async_ipmi_call(conn, yield);
        },
        boost::asio::detached);
    (void)boost::asio::spawn(
        io,
        [conn](boost::asio::yield_context yield) {
            do_start_async_to_yield(conn, yield);
        },
        boost::asio::detached);

    conn->async_method_call(
        [](boost::system::error_code ec, int32_t testValue) {
            if (ec)
            {
                std::cerr << "TestYieldFunction returned error with "
                             "async_method_call (ec = "
                          << ec << ")\n";
                return;
            }
            std::cout << "TestYieldFunction return " << testValue << "\n";
        },
        "xyz.openbmc_project.asio-test", "/xyz/openbmc_project/test",
        "xyz.openbmc_project.test", "TestYieldFunction", int32_t(41));
    io.run();

    return 0;
}

int main(int argc, const char* argv[])
{
    if (argc == 1)
    {
        int pid = fork();
        if (pid == 0)
        {
            return client();
        }
        else if (pid > 0)
        {
            return server();
        }
        return pid;
    }
    if (std::string(argv[1]) == "--server")
    {
        return server();
    }
    if (std::string(argv[1]) == "--client")
    {
        return client();
    }
    std::cout << "usage: " << argv[0] << " [--server | --client]\n";
    return -1;
}
