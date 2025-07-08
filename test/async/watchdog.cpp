#include <sdbusplus/async.hpp>
#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace sdbusplus::async
{
struct WatchdogMock : public sdbusplus::SdBusImpl
{
    WatchdogMock() : sdbusplus::SdBusImpl()
    {
        sd_bus_open(&busp);
    }

    ~WatchdogMock()
    {
        sd_bus_unref(busp);
    }

    MOCK_METHOD(int, sd_notify, (int, const char*), (override));

    MOCK_METHOD(int, sd_watchdog_enabled, (int, uint64_t* usec), (override));

    sdbusplus::bus::busp_t busp = nullptr;
};

struct WatchdogContext : public testing::Test
{
    WatchdogMock sdbusMock;
    sdbusplus::async::context ctx;

    WatchdogContext() : ctx(sdbusplus::bus_t(sdbusMock.busp, &sdbusMock)) {}

    ~WatchdogContext() noexcept override = default;

    void TearDown() override {}

    void spawnStop()
    {
        ctx.spawn(stdexec::just() |
                  stdexec::then([this]() { ctx.request_stop(); }));
    }

    void runToStop()
    {
        spawnStop();
        ctx.run();
    }
};

TEST_F(WatchdogContext, WatchdogEnabledAndHeartbeat)
{
    using namespace testing;
    using namespace std::literals;

    // Expect sd_watchdog_enabled to be called once and return .1 second
    EXPECT_CALL(sdbusMock, sd_watchdog_enabled(_, _))
        .WillOnce([](int, uint64_t* usec) {
            *usec = 100000; // .1 second
            return 0;
        });

    // Expect sd_notify to be called at least once for heartbeat
    // The watchdog_loop divides the time by 2, so it should be
    // called every 0.05 seconds.
    EXPECT_CALL(sdbusMock, sd_notify(_, StrEq("WATCHDOG=1")))
        .WillRepeatedly(Return(0));

    // Run the context for a short period to allow watchdog_loop to execute
    // and send a handful of heartbeats.
    ctx.spawn(sdbusplus::async::sleep_for(ctx, 1s));
    runToStop();

    // The EXPECT_CALL for sd_notify will verify if it was called as expected.
    // If the test passes, it means sd_watchdog_enabled was called,
    // and sd_notify was called for heartbeats.
}

TEST_F(WatchdogContext, WatchdogDisabled)
{
    using namespace testing;
    using namespace std::literals;

    // Expect sd_watchdog_enabled to be called once and return 0 (disabled)
    EXPECT_CALL(sdbusMock, sd_watchdog_enabled(_, _))
        .WillOnce([](int, uint64_t* usec) {
            *usec = 0; // Watchdog disabled
            return 0;
        });

    // Expect sd_notify will NOT be called
    EXPECT_CALL(sdbusMock, sd_notify(_, _)).Times(0);

    // Run the context. No sleep is needed as watchdog should exit immediately.
    runToStop();
}

} // namespace sdbusplus::async
