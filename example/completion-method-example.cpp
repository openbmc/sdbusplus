// Example of a method whose D-Bus reply is completed asynchronously via a
// completion handle, without using a coroutine.
//
// The server registers "CompletionAdd" with register_completion_method.  The
// handler does not compute its result inline; instead it hands the work off
// to an asynchronous operation (here, a steady_timer standing in for any
// callback-style async API) and moves the completion into that
// continuation.  The handler returns immediately, freeing the D-Bus dispatch
// thread, and the reply is sent later when the timer fires.
//
// A periodic "heartbeat" timer on the server prints ticks while the reply is
// outstanding, demonstrating that the io_context keeps running and the call
// is genuinely non-blocking.

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

namespace
{
constexpr auto service = "xyz.openbmc_project.completion-example";
constexpr auto path = "/xyz/openbmc_project/completion";
constexpr auto interface = "xyz.openbmc_project.completion";

// Stand-in for the latency of a real callback-based async backend.
constexpr auto replyDelay = std::chrono::milliseconds(500);
constexpr auto heartbeatPeriod = std::chrono::milliseconds(100);

// Reschedules itself forever, printing a tick each period.  Its purpose is
// purely to show that the server's event loop continues to make progress
// while a CompletionAdd reply is pending.
void startHeartbeat(const std::shared_ptr<boost::asio::steady_timer>& timer)
{
    timer->expires_after(heartbeatPeriod);
    timer->async_wait([timer](const boost::system::error_code& ec) {
        if (ec)
        {
            return;
        }
        std::cout << "server: *** tick *** (event loop still running)\n";
        startHeartbeat(timer);
    });
}
} // namespace

int server()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    conn->request_name(service);
    auto objServer = sdbusplus::asio::object_server(conn);
    std::shared_ptr<sdbusplus::asio::dbus_interface> iface =
        objServer.add_interface(path, interface);

    // CompletionAdd takes an int32 and returns an int32, but the result is not
    // available when the handler returns.  The handler captures io so it can
    // schedule the async continuation, then moves the completion into that
    // continuation and returns void.  No reply is sent yet.
    iface->register_completion_method<int32_t>(
        "CompletionAdd", [&io](sdbusplus::asio::completion c, int32_t value) {
            std::cout << "server: CompletionAdd(" << value
                      << ") received; deferring reply, returning to the "
                         "event loop\n";

            auto work = std::make_shared<boost::asio::steady_timer>(io);
            work->expires_after(replyDelay);
            work->async_wait([work, c = std::move(c), value](
                                 const boost::system::error_code& ec) mutable {
                if (ec)
                {
                    return;
                }
                std::cout << "server: async work done; completing reply\n";
                // Complete the D-Bus reply now, long after the handler
                // returned.
                c.send(value + 1);
            });
        });

    iface->initialize();

    auto heartbeat = std::make_shared<boost::asio::steady_timer>(io);
    startHeartbeat(heartbeat);

    io.run();
    return 0;
}

int client()
{
    boost::asio::io_context io;
    auto conn = std::make_shared<sdbusplus::asio::connection>(io);

    // Issue the call asynchronously.  The client's own event loop is free
    // while the server takes its time producing the reply.
    conn->async_method_call(
        [](boost::system::error_code ec, int32_t value) {
            if (ec)
            {
                std::cerr << "client: CompletionAdd failed: " << ec << "\n";
                return;
            }
            std::cout << "client: CompletionAdd(41) -> " << value << "\n";
        },
        service, path, interface, "CompletionAdd", int32_t(41));

    io.run();
    return 0;
}

int main(int argc, const char* argv[])
{
    if (argc == 2 && std::string(argv[1]) == "--server")
    {
        return server();
    }
    if (argc == 2 && std::string(argv[1]) == "--client")
    {
        return client();
    }

    // Default: fork a server and a client so the example is self-contained.
    int pid = fork();
    if (pid == 0)
    {
        // Give the server a moment to claim its name before calling.
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return client();
    }
    if (pid > 0)
    {
        return server();
    }
    std::cerr << "fork failed\n";
    return -1;
}
