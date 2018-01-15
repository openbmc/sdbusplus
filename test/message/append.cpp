#include <cassert>
#include <iostream>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>

// Global to share the dbus type string between client and server.
static std::string verifyTypeString;

using verifyCallback_t = void (*)(sd_bus_message *);
verifyCallback_t verifyCallback = nullptr;

static constexpr auto SERVICE = "sdbusplus.test.message.append";
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
        auto m = bus.process().release();

        if (m == nullptr)
        {
            bus.wait();
            continue;
        }

        if (sd_bus_message_is_method_call(m, INTERFACE, TEST_METHOD))
        {
            // Verify the message type matches what the test expects.
            assert(verifyTypeString == sd_bus_message_get_signature(m, true));
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
            sd_bus_reply_method_return(m, nullptr);
        }
        else if (sd_bus_message_is_method_call(m, INTERFACE, QUIT_METHOD))
        {
            // Reply and exit.
            sd_bus_reply_method_return(m, nullptr);
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
            static void op(sd_bus_message *m)
            {
                int32_t i = 0;
                sd_bus_message_read_basic(m, 'i', &i);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0, b = 0;
                sd_bus_message_read(m, "ii", &a, &b);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0, b = 0, c = 0, d = 0, e = 0;
                sd_bus_message_read(m, "iiiii", &a, &b, &c, &d, &e);
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
            static void op(sd_bus_message *m)
            {
                bool t1, t2, f1, f2, f3;
                double d;
                sd_bus_message_read(m, "bbbbbd", &t1, &t2, &f1, &f2, &f3, &d);
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
            static void op(sd_bus_message *m)
            {
                const char *s = nullptr;
                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("asdf", s));
            }
        };
        verifyCallback = &verify::op;

        b.call_noreply(m);
    }

    // Test const string owned by const struct.  openbmc/openbmc#1025
    {
        struct
        {
            const char *foo;

            void insert(sdbusplus::message::message &m)
            {
                m.append(foo);
            }
        } s;

        auto m = newMethodCall__test(b);
        s.foo = "1234";
        s.insert(m);

        verifyTypeString = "s";

        struct verify
        {
            static void op(sd_bus_message *m)
            {
                const char *s = nullptr;
                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("1234", s));
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
        const char *str3 = "1234";
        const char *const str4 = "5678";
        const auto str5 = "!@#$";
        m.append(1, "asdf", "ASDF"s, str, std::move(str2), str3, str4, str5, 5);
        verifyTypeString = "isssssssi";

        struct verify
        {
            static void op(sd_bus_message *m)
            {
                int32_t a = 0, b = 0;
                const char *s0 = nullptr, *s1 = nullptr, *s2 = nullptr,
                           *s3 = nullptr, *s4 = nullptr, *s5 = nullptr,
                           *s6 = nullptr;
                sd_bus_message_read(m, "isssssssi", &a, &s0, &s1, &s2, &s3, &s4,
                                    &s5, &s6, &b);
                assert(a == 1);
                assert(b == 5);
                assert(0 == strcmp("asdf", s0));
                assert(0 == strcmp("ASDF", s1));
                assert(0 == strcmp("jkl;", s2));
                assert(0 == strcmp("JKL:", s3));
                assert(0 == strcmp("1234", s4));
                assert(0 == strcmp("5678", s5));
                assert(0 == strcmp("!@#$", s6));
                assert(b == 5);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0, b = 0;
                const char *s0 = nullptr, *s1 = nullptr;
                sd_bus_message_read(m, "iogi", &a, &s0, &s1, &b);
                assert(a == 1);
                assert(b == 4);
                assert(0 == strcmp("/asdf", s0));
                assert(0 == strcmp("iii", s1));
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0;
                sd_bus_message_read(m, "i", &a);
                assert(a == 1);

                auto rc =
                    sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "s");
                assert(0 <= rc);

                const char *s = nullptr;
                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("1", s));
                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("2", s));
                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("3", s));
                assert(1 == sd_bus_message_at_end(m, false));

                sd_bus_message_exit_container(m);

                sd_bus_message_read(m, "i", &a);
                assert(a == 2);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0;
                sd_bus_message_read(m, "i", &a);
                assert(a == 1);

                auto rc = sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY,
                                                         "{si}");
                assert(0 <= rc);

                rc = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                                    "si");
                assert(0 <= rc);

                const char *s = nullptr;
                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("asdf", s));
                sd_bus_message_read_basic(m, 'i', &a);
                assert(a == 3);

                assert(1 == sd_bus_message_at_end(m, false));
                sd_bus_message_exit_container(m);

                rc = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                                    "si");
                assert(0 <= rc);

                sd_bus_message_read_basic(m, 's', &s);
                assert(0 == strcmp("jkl;", s));
                sd_bus_message_read_basic(m, 'i', &a);
                assert(a == 4);

                assert(1 == sd_bus_message_at_end(m, false));
                sd_bus_message_exit_container(m);

                assert(1 == sd_bus_message_at_end(m, false));
                sd_bus_message_exit_container(m);

                sd_bus_message_read(m, "i", &a);
                assert(a == 2);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0;
                double b = 0;
                const char *c = nullptr;

                sd_bus_message_read(m, "i", &a);
                assert(a == 1);

                auto rc = sd_bus_message_enter_container(m, SD_BUS_TYPE_STRUCT,
                                                         "ids");
                assert(0 <= rc);

                sd_bus_message_read(m, "ids", &a, &b, &c);
                assert(a == 3);
                assert(b == 4.1);
                assert(0 == strcmp(c, "asdf"));

                sd_bus_message_exit_container(m);

                sd_bus_message_read(m, "i", &a);
                assert(a == 2);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0;
                double b = 0;

                sd_bus_message_read(m, "i", &a);
                assert(a == 1);

                sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "d");
                sd_bus_message_read(m, "d", &b);
                assert(b == 3.1);
                sd_bus_message_exit_container(m);

                sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "i");
                sd_bus_message_read(m, "i", &a);
                assert(a == 4);
                sd_bus_message_exit_container(m);

                sd_bus_message_read(m, "i", &a);
                assert(a == 2);
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
            static void op(sd_bus_message *m)
            {
                int32_t a = 0;
                double b = 0;
                const char *c = nullptr;

                sd_bus_message_read(m, "i", &a);
                assert(a == 1);

                sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}");
                sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                sd_bus_message_read(m, "s", &c);
                assert(0 == strcmp("asdf", c));
                sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "i");
                sd_bus_message_read(m, "i", &a);
                assert(a == 3);
                sd_bus_message_exit_container(m);
                sd_bus_message_exit_container(m);
                sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv");
                sd_bus_message_read(m, "s", &c);
                assert(0 == strcmp("jkl;", c));
                sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "d");
                sd_bus_message_read(m, "d", &b);
                assert(b == 4.1);
                sd_bus_message_exit_container(m);
                sd_bus_message_exit_container(m);
                sd_bus_message_exit_container(m);

                sd_bus_message_read(m, "i", &a);
                assert(a == 2);
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
