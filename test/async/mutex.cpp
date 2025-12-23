#include <sdbusplus/async.hpp>

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

using namespace std::literals;

namespace fs = std::filesystem;

class MutexTest : public ::testing::Test
{
  protected:
    ~MutexTest() noexcept override = default;
    constexpr static std::string testMutex = "TestMutex";
    const fs::path path = "/tmp";

    MutexTest() : mutex(testMutex)
    {
        auto fd = inotify_init1(IN_NONBLOCK);
        EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                          << errno;

        auto wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
        EXPECT_NE(wd, -1)
            << "Error occurred during the inotify_add_watch, error: " << errno;
        fdioInstance = std::make_unique<sdbusplus::async::fdio>(*ctx, fd);
    }

    std::optional<sdbusplus::async::context> ctx{std::in_place};

    auto testAsyncAddition(int val = 1) -> sdbusplus::async::task<>
    {
        sdbusplus::async::lock_guard lg{mutex};
        co_await lg.lock();

        sharedVar += val;
    }

    auto testAsyncSubtraction(int val = 1) -> sdbusplus::async::task<>
    {
        sdbusplus::async::lock_guard lg{mutex};
        co_await lg.lock();

        sharedVar -= val;
    }

    auto writeToFile() -> sdbusplus::async::task<>
    {
        std::ofstream outfile((path / testMutex).native());
        EXPECT_TRUE(outfile.is_open())
            << "Error occurred during file open, error: " << errno;

        outfile << testMutex << std::endl;
        outfile.close();

        co_return;
    }

    auto readFromFile() -> sdbusplus::async::task<>
    {
        std::ifstream infile((path / testMutex).native());
        EXPECT_TRUE(infile.is_open())
            << "Error occurred during file open, error: " << errno;

        std::string line;
        std::getline(infile, line);
        EXPECT_EQ(line, testMutex);
        infile.close();

        co_return;
    }

    auto testFdEvents() -> sdbusplus::async::task<>
    {
        sdbusplus::async::lock_guard lg{mutex};
        co_await lg.lock();

        ctx->spawn(writeToFile());
        co_await fdioInstance->next();
        co_await readFromFile();
        ran++;

        co_return;
    }

    int sharedVar = 0;
    int ran = 0;
    sdbusplus::async::mutex mutex;

  private:
    std::unique_ptr<sdbusplus::async::fdio> fdioInstance;
    int fd = -1;
    int wd = -1;
};

TEST_F(MutexTest, TestAsyncAddition)
{
    constexpr auto testIterations = 10;
    for (auto i = 0; i < testIterations; i++)
    {
        ctx->spawn(testAsyncAddition());
    }

    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 1s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));

    ctx->run();

    EXPECT_EQ(sharedVar, testIterations);
}

TEST_F(MutexTest, TestAsyncMixed)
{
    constexpr auto testIterations = 10;
    for (auto i = 0; i < testIterations; i++)
    {
        ctx->spawn(testAsyncAddition());
        ctx->spawn(testAsyncSubtraction(2));
    }

    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 1s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));

    ctx->run();

    EXPECT_EQ(sharedVar, -testIterations);
}

TEST_F(MutexTest, TestFdEvents)
{
    constexpr static auto testIterations = 5;

    for (auto i = 0; i < testIterations; i++)
    {
        ctx->spawn(testFdEvents());
    }
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 3s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_EQ(ran, testIterations);
}

TEST_F(MutexTest, TestLockGuardNoLock)
{
    sdbusplus::async::lock_guard lg{mutex};
}
