#include <gtest/gtest.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

class Match : public ::testing::Test
{
    protected:
        decltype(sdbusplus::bus::new_default()) bus =
                sdbusplus::bus::new_default();

        static constexpr auto busName =
                "xyz.openbmc_project.sdbusplus.test.Match";

        static constexpr auto matchRule =
                "type='signal',"
                "interface=org.freedesktop.DBus,"
                "member='NameOwnerChanged',"
                "arg0='xyz.openbmc_project.sdbusplus.test.Match'";

        void waitForIt(bool& triggered)
        {
            for (size_t i = 0; (i < 16) && !triggered; ++i)
            {
                bus.wait(0);
                bus.process_discard();
            }
        }
};

TEST_F(Match, FunctorIs_sd_bus_message_handler_t)
{
    using namespace std::literals;

    bool triggered = false;
    auto trigger = [](sd_bus_message *m, void* context, sd_bus_error* e)
        {
            *static_cast<bool*>(context) = true;
            return 0;
        };

    sdbusplus::bus::match_t m{bus, matchRule, trigger, &triggered};
    auto m2 = std::move(m);  // ensure match is move-safe.

    waitForIt(triggered);
    ASSERT_FALSE(triggered);

    bus.request_name(busName);

    waitForIt(triggered);
    ASSERT_TRUE(triggered);
}
