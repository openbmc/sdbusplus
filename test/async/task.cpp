#include <sdbusplus/async/execution.hpp>
#include <sdbusplus/async/task.hpp>

#include <gtest/gtest.h>

using namespace sdbusplus::async;

TEST(Task, CoAwaitVoid)
{
    bool value = false;
    auto t = [&]() -> task<> {
        value = true;
        co_return;
    };

    // Check to ensure the co-routine hasn't started executing yet.
    EXPECT_FALSE(value);

    // Run it and confirm the value is updated.
    std::this_thread::sync_wait(t());
    EXPECT_TRUE(value);
}

TEST(Task, CoAwaitInt)
{
    struct _
    {
        static auto one() -> task<int>
        {
            co_return 42;
        }
        static auto two(bool& executed) -> task<>
        {
            auto r = co_await one();
            EXPECT_EQ(r, 42);
            executed = true;
            co_return;
        }
    };

    // Add boolean to confirm that co-routine actually executed by the
    // end.
    bool executed = false;
    std::this_thread::sync_wait(_::two(executed));
    EXPECT_TRUE(executed);
}

TEST(Task, CoAwaitThrow)
{
    struct _
    {
        static auto one() -> task<>
        {
            throw std::logic_error("Failed");
            co_return;
        }

        static auto two(bool& caught) -> task<>
        {
            try
            {
                co_await (one());
            }
            catch (const std::logic_error&)
            {
                caught = true;
            }
        }
    };

    // Ensure throws surface up.
    EXPECT_THROW(std::this_thread::sync_wait(_::one()), std::logic_error);

    // Ensure throws can be caught inside a co-routine.
    bool caught = false;
    std::this_thread::sync_wait(_::two(caught));
    EXPECT_TRUE(caught);
}
