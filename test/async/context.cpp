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

    void runToStop()
    {
        ctx->run(std::execution::just() |
                 std::execution::then([this]() { ctx->request_stop(); }));
    }

    std::optional<sdbusplus::async::context> ctx{std::in_place};
};

TEST_F(Context, RunSimple)
{
    runToStop();
}

TEST_F(Context, SpawnedTask)
{
    ctx->spawn(std::execution::just());
    runToStop();
}

TEST_F(Context, SpawnDelayedTask)
{
    using namespace std::literals;
    static constexpr auto timeout = 500ms;

    auto start = std::chrono::steady_clock::now();

    bool ran = false;
    ctx->spawn(sdbusplus::async::sleep_for(*ctx, timeout) |
               std::execution::then([&ran]() { ran = true; }));

    runToStop();

    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(ran);
    EXPECT_GT(stop - start, timeout);
    EXPECT_LT(stop - start, timeout * 2);
}

TEST_F(Context, DestructMatcherWithPendingAwait)
{
    bool ran = false;
    ctx->spawn(std::execution::just() | std::execution::then([this, &ran]() {
                   // Create a bogus match.
                   sdbusplus::async::match m{
                       *ctx, sdbusplus::bus::match::rules::interfacesAdded(
                                 "/this/is/a/bogus/path/for/SpawnMatcher")};

                   // Await the match completion (which will never happen).
                   ctx->spawn(m.next() | std::execution::then(
                                             [&ran](...) { ran = true; }));

                   // Destruct the match.
               }));

    ctx->spawn(std::execution::just());

    EXPECT_THROW(runToStop(), sdbusplus::exception::UnhandledStop);
    EXPECT_FALSE(ran);
}

TEST_F(Context, DestructMatcherWithPendingAwaitAsTask)
{
    using namespace std::literals;

    auto m = std::make_optional<sdbusplus::async::match>(
        *ctx, sdbusplus::bus::match::rules::interfacesAdded(
                  "/this/is/a/bogus/path/for/SpawnMatcher"));

    struct _
    {
        static auto fn(decltype(m->next())&& snd, bool& ran)
            -> sdbusplus::async::task<>
        {
            co_await std::move(snd);
            ran = true;
            co_return;
        }
    };

    bool ran = false;
    ctx->spawn(_::fn(m->next(), ran));
    ctx->spawn(sdbusplus::async::sleep_for(*ctx, 1ms) |
               std::execution::then([&]() { m.reset(); }));

    EXPECT_THROW(runToStop(), sdbusplus::exception::UnhandledStop);
    EXPECT_FALSE(ran);
}
