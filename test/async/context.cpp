#include <sdbusplus/async.hpp>

#include <gtest/gtest.h>

struct Context : public testing::Test
{
    ~Context() noexcept override = default;

    void TearDown() override
    {
        // Destructing the context can throw, so we have to do it in
        // the TearDown in order to make our destructor noexcept.
        ctx.reset();
    }

    // Local accessor that makes clang-tidy's flow analysis happy.
    // It provides a local guard that the optional is engaged.
    sdbusplus::async::context& Ctx()
    {
        if (!ctx.has_value())
        {
            throw std::logic_error("Context is not initialized");
        }
        return ctx.value();
    }

    void SetUp() override
    {
        ctx.emplace();
    }

    void spawnStop()
    {
        Ctx().spawn(stdexec::just() |
                    stdexec::then([this]() { Ctx().request_stop(); }));
    }

    void runToStop()
    {
        spawnStop();
        Ctx().run();
    }

    std::optional<sdbusplus::async::context> ctx{std::in_place};
};

TEST_F(Context, RunSimple)
{
    runToStop();
}

TEST_F(Context, SpawnedTask)
{
    Ctx().spawn(stdexec::just());
    runToStop();
}

TEST_F(Context, ReentrantRun)
{
    runToStop();
    for (int i = 0; i < 100; ++i)
    {
        Ctx().run();
    }
}

TEST_F(Context, SpawnDelayedTask)
{
    using namespace std::literals;
    static constexpr auto timeout = 500ms;

    auto start = std::chrono::steady_clock::now();

    bool ran = false;
    Ctx().spawn(sdbusplus::async::sleep_for(Ctx(), timeout) |
                stdexec::then([&ran]() { ran = true; }));

    runToStop();

    auto stop = std::chrono::steady_clock::now();

    EXPECT_TRUE(ran);
    EXPECT_GT(stop - start, timeout);
    EXPECT_LT(stop - start, timeout * 3);
}

TEST_F(Context, SpawnRecursiveTask)
{
    struct _
    {
        static auto one(size_t count, size_t& executed)
            -> sdbusplus::async::task<size_t>
        {
            if (count)
            {
                ++executed;
                co_return (co_await one(count - 1, executed)) + 1;
            }
            co_return co_await stdexec::just(0);
        }
    };

    static constexpr size_t count = 100;
    size_t executed = 0;

    Ctx().spawn(_::one(count, executed) |
                stdexec::then([=](auto result) { EXPECT_EQ(result, count); }));

    runToStop();

    EXPECT_EQ(executed, count);
}

TEST_F(Context, DestructMatcherWithPendingAwait)
{
    using namespace std::literals;

    bool ran = false;
    auto m = std::make_optional<sdbusplus::async::match>(
        Ctx(), sdbusplus::bus::match::rules::interfacesAdded(
                   "/this/is/a/bogus/path/for/SpawnMatcher"));

    // Await the match completion (which will never happen).
    Ctx().spawn(m->next() | stdexec::then([&ran](...) { ran = true; }));

    // Destruct the match.
    Ctx().spawn(sdbusplus::async::sleep_for(Ctx(), 1ms) |
                stdexec::then([&m](...) { m.reset(); }));

    runToStop();
    EXPECT_FALSE(ran);
}

TEST_F(Context, DestructMatcherWithPendingAwaitAsTask)
{
    using namespace std::literals;

    auto m = std::make_optional<sdbusplus::async::match>(
        Ctx(), sdbusplus::bus::match::rules::interfacesAdded(
                   "/this/is/a/bogus/path/for/SpawnMatcher"));

    struct _
    {
        static auto fn(decltype(m->next()) snd, bool& ran)
            -> sdbusplus::async::task<>
        {
            co_await std::move(snd);
            ran = true;
            co_return;
        }
    };

    bool ran = false;
    Ctx().spawn(_::fn(m->next(), ran));
    Ctx().spawn(sdbusplus::async::sleep_for(Ctx(), 1ms) |
                stdexec::then([&]() { m.reset(); }));

    runToStop();
    EXPECT_FALSE(ran);
}
