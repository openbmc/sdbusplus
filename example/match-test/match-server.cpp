#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>
#include <thread>

std::shared_ptr<sdbusplus::asio::dbus_interface> fooIfc;
static std::string fooProperty = "foo";
boost::asio::io_context io;
auto conn = std::make_shared<sdbusplus::asio::connection>(io);

std::optional<sdbusplus::bus::match_t> match;

void registerMatch()
{
    match = sdbusplus::bus::match_t(
        *conn,
        sdbusplus::bus::match::rules::propertiesChanged("/com/foo", "com.foo"),
        [](sdbusplus::message_t&) {});
}

void timerHandler()
{
    // Create match_t frequently to demo the issue
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

int main(int /*argc*/, char** /*argv*/)
{
    sdbusplus::asio::object_server objectServer(conn);

    fooIfc = objectServer.add_interface("/com/foo", "com.foo");

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

    conn->request_name("com.foo");

    io.run();

    return 0;
}
