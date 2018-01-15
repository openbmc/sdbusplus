#include <gtest/gtest.h>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/bus/match.hpp>

class Match : public ::testing::Test
{
  protected:
    decltype(sdbusplus::bus::new_default()) bus = sdbusplus::bus::new_default();

    static constexpr auto busName = "xyz.openbmc_project.sdbusplus.test.Match";

    static auto matchRule()
    {
        using namespace sdbusplus::bus::match::rules;
        return nameOwnerChanged() + argN(0, busName);
    }

    void waitForIt(bool &triggered)
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
    bool triggered = false;
    auto trigger = [](sd_bus_message *m, void *context, sd_bus_error *e) {
        *static_cast<bool *>(context) = true;
        return 0;
    };

    sdbusplus::bus::match_t m{bus, matchRule(), trigger, &triggered};
    auto m2 = std::move(m); // ensure match is move-safe.

    waitForIt(triggered);
    ASSERT_FALSE(triggered);

    bus.request_name(busName);

    waitForIt(triggered);
    ASSERT_TRUE(triggered);
}

TEST_F(Match, FunctorIs_LambdaTakingMessage)
{
    bool triggered = false;
    auto trigger = [&triggered](sdbusplus::message::message &m) {
        triggered = true;
    };

    sdbusplus::bus::match_t m{bus, matchRule(), trigger};
    auto m2 = std::move(m); // ensure match is move-safe.

    waitForIt(triggered);
    ASSERT_FALSE(triggered);

    bus.request_name(busName);

    waitForIt(triggered);
    ASSERT_TRUE(triggered);
}

TEST_F(Match, FunctorIs_MemberFunctionTakingMessage)
{

    class BoolHolder
    {
      public:
        bool triggered = false;

        void callback(sdbusplus::message::message &m)
        {
            triggered = true;
        }
    };
    BoolHolder b;

    sdbusplus::bus::match_t m{bus, matchRule(),
                              std::bind(std::mem_fn(&BoolHolder::callback), &b,
                                        std::placeholders::_1)};
    auto m2 = std::move(m); // ensure match is move-safe.

    waitForIt(b.triggered);
    ASSERT_FALSE(b.triggered);

    bus.request_name(busName);

    waitForIt(b.triggered);
    ASSERT_TRUE(b.triggered);
}
