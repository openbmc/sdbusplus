#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/match.hpp>
#include <sdbusplus/bus/match.hpp>

#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::MatchesRegex;

TEST(MatchAsio, Basic)
{
    static constexpr auto busName =
        "xyz.openbmc_project.sdbusplus.test.MatchAsync";

    boost::asio::io_context io;
    sdbusplus::asio::connection conn(io);

    bool triggered = false;
    auto on_event = [&triggered, &io](sdbusplus::asio::WatchEventType err,
                                      sdbusplus::message_t& m) {
        if (err == sdbusplus::asio::WatchEventType::Message)
        {
            triggered = true;
            io.stop();

            std::string name;
            std::string old_owner;
            std::string new_owner;
            m.read(name, old_owner, new_owner);
            EXPECT_EQ(name, busName);
            EXPECT_EQ(old_owner, "");
            EXPECT_THAT(new_owner, MatchesRegex(":[0-9]\\.[0-9]{1,5}"));
        }
    };

    std::string match = sdbusplus::bus::match::rules::nameOwnerChanged() +
                        sdbusplus::bus::match::rules::argN(0, busName);
    sdbusplus::asio::match m(conn, match.c_str(), std::move(on_event));

    conn.request_name(busName);

    io.run_for(std::chrono::seconds(60));
    EXPECT_TRUE(triggered);
}
