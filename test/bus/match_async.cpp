#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus/match.hpp>

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

TEST_F(MatchAsync, CreateWithAsio)
{
    bool triggered = false;
    auto trigger = [](sd_bus_message* /*m*/, void* context,
                      sd_bus_error* /*e*/) {
        *static_cast<bool*>(context) = true;
        return 0;
    };

    sdbusplus::bus::match_t m{*conn, matchRule(), trigger, &triggered};
    auto m2 = std::move(m); // ensure match is move-safe.

    conn->request_name(busName);

    waitWithAsio(triggered);
    ASSERT_TRUE(triggered);
}
