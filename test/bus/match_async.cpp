#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/match.hpp>

#include <chrono>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

class MatchAsync : public ::testing::Test
{
  protected:
    boost::asio::io_context io;
    std::shared_ptr<sdbusplus::asio::connection> conn;

    static constexpr auto busName =
        "xyz.openbmc_project.sdbusplus.test.MatchAsync";

    MatchAsync()
    {
        conn = std::make_shared<sdbusplus::asio::connection>(io);
    }

    static auto matchRule()
    {
        using namespace sdbusplus::bus::match::rules;
        return nameOwnerChanged() + argN(0, busName);
    }

    void waitWithAsio(bool& triggered)
    {
        for (size_t i = 0; (i < 16) && !triggered; ++i)
        {
            io.run_one_for(10ms);
        }
    }
};

TEST_F(MatchAsync, CreateWithAsioWithEventCallbackOnly)
{
    bool triggered = false;
    auto trigger = [&triggered](sdbusplus::message_t&) { triggered = true; };

    sdbusplus::asio::match m{*conn, matchRule(), trigger};
    // The asio::match is not movable

    conn->request_name(busName);

    waitWithAsio(triggered);
    ASSERT_TRUE(triggered);
}

TEST_F(MatchAsync, CreateWithAsioWithBothEventAndInstallCallback)
{
    bool triggered = false;
    auto trigger = [&triggered](sdbusplus::message_t&) { triggered = true; };

    bool installed = false;
    auto installCallback = [&installed](sdbusplus::message_t&) {
        installed = true;
    };

    sdbusplus::asio::match m{*conn, matchRule(), trigger, installCallback};
    // The asio::match is not movable

    conn->request_name(busName);

    waitWithAsio(installed);
    ASSERT_TRUE(installed);

    waitWithAsio(triggered);
    ASSERT_TRUE(triggered);
}
