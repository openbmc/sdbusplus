#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/match.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>

constexpr auto fooProperty = "foo";
constexpr auto service = "xyz.openbmc_project.MatchIssueDemo";
constexpr auto path = "/xyz/openbmc_project/match_issue_demo";
constexpr auto interface = service;

boost::asio::io_context io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

std::unique_ptr<sdbusplus::asio::match> asioMatch;

void registerMatch()
{
    asioMatch = std::make_unique<sdbusplus::asio::match>(
        *conn, sdbusplus::bus::match::rules::propertiesChanged(path, interface),
        [](sdbusplus::message_t&) {});
}

void timerHandler()
{
    // Create match frequently to demo the issue
    static boost::asio::steady_timer timer(io);
    timer.expires_after(std::chrono::milliseconds(10));
    timer.async_wait([](const boost::system::error_code& ec) {
        if (ec)
        {
            return;
        }
        registerMatch();
        timerHandler();
    });
}

int main()
{
    sdbusplus::asio::object_server objectServer(conn);

    auto fooIfc = objectServer.add_interface(path, interface);

    fooIfc->register_property(
        "Foo", std::string("foo"),
        // setter
        [](const std::string& req, std::string& propertyValue) {
        propertyValue = req;
        return 1; // Success
    },
        // getter
        [](const std::string& foo) { return foo; });
    fooIfc->initialize();

    timerHandler();

    conn->request_name(interface);

    io.run();

    return 0;
}
