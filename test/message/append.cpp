#include <iostream>
#include <sdbusplus/message/append.hpp>
#include <cassert>
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

void newMethodCall__test(sdbusplus::bus::bus& b, sd_bus_message** m)
{
    // Allocate a method-call message for INTERFACE,TEST_METHOD.
    *m = b.new_method_call(SERVICE, "/", INTERFACE, TEST_METHOD);
}

void runTests()
{
    using namespace std::literals;

    sd_bus_message* m = nullptr;
    auto b = sdbusplus::bus::new_default();

    // Test r-value int.
    {
        newMethodCall__test(b, &m);
        sdbusplus::message::append(m, 1);
        verifyTypeString = "i";
        b.call_noreply(m);
    }

    // Test l-value int.
    {
        newMethodCall__test(b, &m);
        int a = 1;
        sdbusplus::message::append(m, a, a);
        verifyTypeString = "ii";
        b.call_noreply(m);
    }

    // Test multiple ints.
    {
        newMethodCall__test(b, &m);
        sdbusplus::message::append(m, 1, 2, 3, 4, 5);
        verifyTypeString = "iiiii";
        b.call_noreply(m);
    }

    // Test r-value string.
    {
        newMethodCall__test(b, &m);
        sdbusplus::message::append(m, "asdf"s);
        verifyTypeString = "s";
        b.call_noreply(m);
    }

    // Test multiple strings, various forms.
    {
        newMethodCall__test(b, &m);
        auto str = "jkl;"s;
        auto str2 = "JKL:"s;
        sdbusplus::message::append(m, 1, "asdf", "ASDF"s, str,
                                   std::move(str2), 5);
        verifyTypeString = "issssi";
        b.call_noreply(m);
    }

    // Shutdown server.
    m = b.new_method_call(SERVICE, "/", INTERFACE, QUIT_METHOD);
    b.call_noreply(m);
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
