#include <iostream>
#include <cassert>
#include <sdbusplus/message.hpp>
#include <sdbusplus/bus.hpp>

// Global to share the dbus type string between client and server.
static std::string verifyTypeString;

static constexpr auto SERVICE = "sdbusplus.test";
static constexpr auto INTERFACE = SERVICE;
static constexpr auto TEST_METHOD = "test";
static constexpr auto QUIT_METHOD = "quit";

// Open up the sdbus and claim SERVICE name.
auto serverInit()
{
    auto b = sdbusplus::bus::new_default();
    b.request_name(SERVICE);

    return std::move(b);
}

// Thread to run the dbus server.
void* server(void* b)
{
    auto bus = sdbusplus::bus::bus(reinterpret_cast<sdbusplus::bus::busp_t>(b));

    while(1)
    {
        // Wait for messages.
        sd_bus_message *m = bus.process();

        if(m == nullptr)
        {
            bus.wait();
            continue;
        }

        if (sd_bus_message_is_method_call(m, INTERFACE, TEST_METHOD))
        {
            // Verify the message type matches what the test expects.
            // TODO: It would be nice to verify content here as well.
            assert(verifyTypeString == sd_bus_message_get_signature(m, true));
            // Reply to client.
            sd_bus_reply_method_return(m, nullptr);
        }
        else if (sd_bus_message_is_method_call(m, INTERFACE, QUIT_METHOD))
        {
            // Reply and exit.
            sd_bus_reply_method_return(m, nullptr);
            break;
        }
    }
}

auto newMethodCall__test(sdbusplus::bus::bus& b)
{
    // Allocate a method-call message for INTERFACE,TEST_METHOD.
    return b.new_method_call(SERVICE, "/", INTERFACE, TEST_METHOD);
}

void runTests()
{
    using namespace std::literals;

    auto b = sdbusplus::bus::new_default();

    // Test r-value int.
    {
        auto m = newMethodCall__test(b);
        m.append(1);
        verifyTypeString = "i";
        b.call_noreply(m);
    }
    // Test l-value int.
    {
        auto m = newMethodCall__test(b);
        int a = 1;
        m.append(a, a);
        verifyTypeString = "ii";
        b.call_noreply(m);
    }

    // Test multiple ints.
    {
        auto m = newMethodCall__test(b);
        m.append(1, 2, 3, 4, 5);
        verifyTypeString = "iiiii";
        b.call_noreply(m);
    }

    // Test r-value string.
    {
        auto m = newMethodCall__test(b);
        m.append("asdf"s);
        verifyTypeString = "s";
        b.call_noreply(m);
    }

    // Test multiple strings, various forms.
    {
        auto m = newMethodCall__test(b);
        auto str = "jkl;"s;
        auto str2 = "JKL:"s;
        m.append(1, "asdf", "ASDF"s, str,
                 std::move(str2), 5);
        verifyTypeString = "issssi";
        b.call_noreply(m);
    }

    // Shutdown server.
    {
        auto m = b.new_method_call(SERVICE, "/", INTERFACE, QUIT_METHOD);
        b.call_noreply(m);
    }
}

int main()
{
    // Initialize and start server thread.
    pthread_t t;
    {
        auto b = serverInit();
        pthread_create(&t, NULL, server, b.release());
    }

    runTests();

    // Wait for server thread to exit.
    pthread_join(t, NULL);

    return 0;
}
