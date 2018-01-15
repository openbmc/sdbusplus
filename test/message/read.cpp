#include <cassert>
#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

// Global to share the dbus type string between client and server.
static std::string verifyTypeString;

using verifyCallback_t = void (*)(sdbusplus::message::message &);
verifyCallback_t verifyCallback = nullptr;

static constexpr auto SERVICE = "sdbusplus.test.message.read";
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
void *server(void *b)
{
    auto bus = sdbusplus::bus::bus(reinterpret_cast<sdbusplus::bus::busp_t>(b));

    while (1)
    {
        // Wait for messages.
        auto m = bus.process();

        if (!m)
        {
            bus.wait();
            continue;
        }

        if (m.is_method_call(INTERFACE, TEST_METHOD))
        {
            // Verify the message type matches what the test expects.
            assert(verifyTypeString == m.get_signature());

            if (verifyCallback)
            {

                verifyCallback(m);
                verifyCallback = nullptr;
            }
            else
            {
                std::cout << "Warning: No verification for " << verifyTypeString
                          << std::endl;
            }
            // Reply to client.
            sd_bus_reply_method_return(m.release(), nullptr);
        }
        else if (m.is_method_call(INTERFACE, QUIT_METHOD))
        {
            // Reply and exit.
            sd_bus_reply_method_return(m.release(), nullptr);
            break;
        }
    }

    return nullptr;
}

auto newMethodCall__test(sdbusplus::bus::bus &b)
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

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t i = 0;
                m.read(i);
                assert(i == 1);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }
    // Test l-value int.
    {
        auto m = newMethodCall__test(b);
        int a = 1;
        m.append(a, a);
        verifyTypeString = "ii";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0;
                m.read(a, b);
                assert(a == 1);
                assert(b == 1);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test multiple ints.
    {
        auto m = newMethodCall__test(b);
        m.append(1, 2, 3, 4, 5);
        verifyTypeString = "iiiii";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0, c = 0, d = 0, e = 0;
                m.read(a, b, c, d, e);
                assert(a == 1);
                assert(b == 2);
                assert(c == 3);
                assert(d == 4);
                assert(e == 5);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test double and bool.
    {
        auto m = newMethodCall__test(b);
        bool t = true;
        bool f = false;
        bool f2 = false;
        m.append(t, true, f, std::move(f2), false, 1.1);
        verifyTypeString = "bbbbbd";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                bool t1, t2, f1, f2, f3;
                double d;
                m.read(t1, t2, f1, f2, f3, d);
                assert(t1);
                assert(t2);
                assert(!f1);
                assert(!f2);
                assert(!f3);
                assert(d == 1.1);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test r-value string.
    {
        auto m = newMethodCall__test(b);
        m.append("asdf"s);
        verifyTypeString = "s";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                const char *s = nullptr;
                m.read(s);
                assert(0 == strcmp("asdf", s));
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test multiple strings, various forms.
    {
        auto m = newMethodCall__test(b);
        auto str = "jkl;"s;
        auto str2 = "JKL:"s;
        m.append(1, "asdf", "ASDF"s, str, std::move(str2), 5);
        verifyTypeString = "issssi";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0;
                std::string s0, s1, s2, s3;
                m.read(a, s0, s1, s2, s3, b);
                assert(a == 1);
                assert(b == 5);
                assert(s0 == "asdf"s);
                assert(s1 == "ASDF"s);
                assert(s2 == "jkl;"s);
                assert(s3 == "JKL:");
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test object_path and signature.
    {
        auto m = newMethodCall__test(b);
        auto o = sdbusplus::message::object_path("/asdf");
        auto s = sdbusplus::message::signature("iii");
        m.append(1, o, s, 4);
        verifyTypeString = "iogi";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0;
                sdbusplus::message::object_path o;
                sdbusplus::message::signature s;
                m.read(a, o, s, b);
                assert(a == 1);
                assert(b == 4);
                assert(std::string(o) == "/asdf"s);
                assert(std::string(s) == "iii"s);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test vector.
    {
        auto m = newMethodCall__test(b);
        std::vector<std::string> s{"1", "2", "3"};
        m.append(1, s, 2);
        verifyTypeString = "iasi";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0;
                std::vector<std::string> s;
                m.read(a, s);
                assert(a == 1);
                assert(s[0] == "1");
                assert(s[1] == "2");
                assert(s[2] == "3");
                decltype(s) s2 = {"1", "2", "3"};
                assert(s == s2);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test map.
    {
        auto m = newMethodCall__test(b);
        std::map<std::string, int> s = {{"asdf", 3}, {"jkl;", 4}};
        m.append(1, s, 2);
        verifyTypeString = "ia{si}i";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0;
                std::map<std::string, int> s{};

                m.read(a, s, b);
                assert(a == 1);
                assert(s.size() == 2);
                assert(s["asdf"] == 3);
                assert(s["jkl;"] == 4);
                assert(b == 2);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test tuple.
    {
        auto m = newMethodCall__test(b);
        std::tuple<int, double, std::string> a{3, 4.1, "asdf"};
        m.append(1, a, 2);
        verifyTypeString = "i(ids)i";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0;
                std::tuple<int, double, std::string> c{};

                m.read(a, c, b);
                assert(a == 1);
                assert(b == 2);
                assert(c == std::make_tuple(3, 4.1, "asdf"s));
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test variant.
    {
        auto m = newMethodCall__test(b);
        sdbusplus::message::variant<int, double> a1{3.1}, a2{4};
        m.append(1, a1, a2, 2);
        verifyTypeString = "ivvi";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a, b;
                sdbusplus::message::variant<int, double> a1{}, a2{};

                m.read(a, a1, a2, b);
                assert(a == 1);
                assert(a1 == 3.1);
                assert(a2 == 4);
                assert(b == 2);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test variant with missing/wrong type.
    {
        auto m = newMethodCall__test(b);
        sdbusplus::message::variant<uint64_t, double> a1{3.1}, a2{uint64_t(4)};
        m.append(1, a1, a2, 2);
        verifyTypeString = "ivvi";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a, b;
                sdbusplus::message::variant<uint8_t, double> a1{}, a2{};

                m.read(a, a1, a2, b);
                assert(a == 1);
                assert(a1 == 3.1);
                assert(a2 == uint8_t());
                assert(b == 2);
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test map-variant.
    {
        auto m = newMethodCall__test(b);
        std::map<std::string, sdbusplus::message::variant<int, double>> a1 = {
            {"asdf", 3}, {"jkl;", 4.1}};
        m.append(1, a1, 2);
        verifyTypeString = "ia{sv}i";

        struct verify
        {
            static void op(sdbusplus::message::message &m)
            {
                int32_t a = 0, b = 0;
                std::map<std::string, sdbusplus::message::variant<int, double>>
                    a1{};

                m.read(a, a1, b);
                assert(a == 1);
                assert(a1["asdf"] == 3);
                assert(a1["jkl;"] == 4.1);
                assert(b == 2);
            }
        };
        verifyCallback = &verify::op;

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
