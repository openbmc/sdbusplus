#include <iostream>
#include <sdbusplus/message/append.hpp>
#include <cassert>

// Global to share the dbus type string between client and server.
static std::string verifyTypeString;

static constexpr auto SERVICE = "sdbusplus.test";
static constexpr auto INTERFACE = SERVICE;
static constexpr auto TEST_METHOD = "test";
static constexpr auto QUIT_METHOD = "quit";

// Open up the sdbus and claim SERVICE name.
void serverInit(sd_bus** b)
{
    assert(0 <= sd_bus_open(b));
    assert(0 <= sd_bus_request_name(*b, SERVICE, 0));
}

// Thread to run the dbus server.
void* server(void* b)
{
    auto bus = reinterpret_cast<sd_bus*>(b);

    while(1)
    {
        // Wait for messages.
        sd_bus_message *m = nullptr;
        if (0 == sd_bus_process(bus, &m))
        {
            sd_bus_wait(bus, 0);
            continue;
        }

        if(!m)
        {
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

void newMethodCall__test(sd_bus* b, sd_bus_message** m)
{
    // Allocate a method-call message for INTERFACE,TEST_METHOD.
    assert(0 <= sd_bus_message_new_method_call(b, m, SERVICE, "/", INTERFACE,
                                               TEST_METHOD));
    sd_bus_message_set_expect_reply(*m, true);
}

void runTests()
{
    using namespace std::literals;

    sd_bus_message* m = nullptr;
    sd_bus* b = nullptr;

    // Connect to dbus.
    assert(0 <= sd_bus_open(&b));

    // Test r-value int.
    {
        newMethodCall__test(b, &m);
        sdbusplus::message::append(m, 1);
        verifyTypeString = "i";
        sd_bus_call(b, m, 0, nullptr, nullptr);
    }

    // Test l-value int.
    {
        newMethodCall__test(b, &m);
        int a = 1;
        sdbusplus::message::append(m, a, a);
        verifyTypeString = "ii";
        sd_bus_call(b, m, 0, nullptr, nullptr);
    }

    // Test multiple ints.
    {
        newMethodCall__test(b, &m);
        sdbusplus::message::append(m, 1, 2, 3, 4, 5);
        verifyTypeString = "iiiii";
        sd_bus_call(b, m, 0, nullptr, nullptr);
    }

    // Test r-value string.
    {
        newMethodCall__test(b, &m);
        sdbusplus::message::append(m, "asdf"s);
        verifyTypeString = "s";
        sd_bus_call(b, m, 0, nullptr, nullptr);
    }

    // Test multiple strings, various forms.
    {
        newMethodCall__test(b, &m);
        auto str = "jkl;"s;
        auto str2 = "JKL:"s;
        sdbusplus::message::append(m, 1, "asdf", "ASDF"s, str,
                                   std::move(str2), 5);
        verifyTypeString = "issssi";
        sd_bus_call(b, m, 0, nullptr, nullptr);
    }

    // Shutdown server.
    sd_bus_call_method(b, SERVICE, "/", INTERFACE, QUIT_METHOD,
                       nullptr, nullptr, nullptr);
}

int main()
{
    // Initialize and start server thread.
    pthread_t t;
    {
        sd_bus* b;
        serverInit(&b);
        pthread_create(&t, NULL, server, b);
    }

    runTests();

    // Wait for server thread to exit.
    pthread_join(t, NULL);

    return 0;
}
