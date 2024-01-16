#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <iostream>
#include <variant>

std::shared_ptr<sdbusplus::asio::dbus_interface> fooIfc;
static std::string fooProperty = "foo";
boost::asio::io_context io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

void syncCall()
{
    constexpr auto service = "xyz.openbmc_project.MatchIssueDemo";
    constexpr auto path = "/xyz/openbmc_project/match_issue_demo";
    constexpr auto interface = service;
    constexpr auto property = "Foo";
    std::variant<std::string> msg;

    auto& bus = *conn;
    auto method = bus.new_method_call(service, path,
                                      "org.freedesktop.DBus.Properties", "Get");
    method.append(interface, property);
    auto reply = bus.call(method);
    reply.read(msg);

    std::string ret = std::get<std::string>(msg);
    auto now = std::chrono::system_clock::now();
    std::cout << std::chrono::system_clock::to_time_t(now)
              << ": Get foo property: " << ret << std::endl;
    // if fail, throw exception, don't catch, will exit
}

void syncTimer()
{
    static boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::milliseconds(10));
    timer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        {
            return;
        }
        syncCall();
        syncTimer();
    });
}

int main()
{
    syncTimer();

    io.run();

    return 0;
}
