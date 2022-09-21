#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

struct Context : public testing::Test
{
    ~Context() noexcept = default;

    void TearDown() override
    {
        // Destructing the context can throw, so we have to do it in
        // the TearDown in order to make our destructor noexcept.
        ctx.reset();
    }

    std::optional<sdbusplus::async::context> ctx{std::in_place};
};

TEST_F(Context, RunSimple)
{
    ctx->run(std::execution::just() |
             std::execution::then([this]() { ctx->request_stop(); }));
}

TEST_F(Context, SpawnedTask)
{
    ctx->spawn(std::execution::just());

    ctx->run(std::execution::just() |
             std::execution::then([this]() { ctx->request_stop(); }));
}

TEST_F(Context, SpawnDelayedTask)
{
    using namespace std::literals;
    static constexpr auto timeout = 500ms;

    auto start = std::chrono::steady_clock::now();

    bool ran = false;
    ctx->spawn(sdbusplus::async::sleep_for(*ctx, timeout) |
               std::execution::then([&ran]() { ran = true; }));

    ctx->run(std::execution::just() |
             std::execution::then([this]() { ctx->request_stop(); }));

    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(ran);
    EXPECT_GT(stop - start, timeout);
    EXPECT_LT(stop - start, timeout * 2);
}
