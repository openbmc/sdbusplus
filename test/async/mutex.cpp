#include <sdbusplus/async.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

class MutexTest : public ::testing::Test
{
  protected:
    ~MutexTest() noexcept override = default;
    constexpr static std::string testMutex = "TestMutex";
    const fs::path path = "/tmp";

    std::unique_ptr<sdbusplus::async::context> ctx;

    MutexTest() :
        ctx(std::make_unique<sdbusplus::async::context>()), mutex(testMutex)
    {
        auto fd = inotify_init1(IN_NONBLOCK);
        EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                          << errno;

        auto wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
        EXPECT_NE(wd, -1)
            << "Error occurred during the inotify_add_watch, error: " << errno;
        fdioInstance = std::make_unique<sdbusplus::async::fdio>(*ctx, fd);
    }

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

        // Write the file concurrent with waiting for the fd event so the
        // notification is not missed.
        co_await sdbusplus::async::execution::when_all(writeToFile(),
                                                       fdioInstance->next());
        co_await readFromFile();
        ran++;

        co_return;
    }

    int sharedVar = 0;
    std::size_t ran = 0;
    sdbusplus::async::mutex mutex;

  private:
    std::unique_ptr<sdbusplus::async::fdio> fdioInstance;
};

TEST_F(MutexTest, TestAsyncAddition)
{
    constexpr std::size_t testIterations = 10;
    std::size_t completed = 0;
    for (std::size_t i = 0; i < testIterations; i++)
    {
        ctx->spawn(
            testAsyncAddition() | sdbusplus::async::execution::then([&]() {
                const std::size_t done = ++completed;
                if (done == testIterations)
                {
                    ctx->request_stop();
                }
            }));
    }

    ctx->run();

    EXPECT_EQ(sharedVar, static_cast<int>(testIterations));
    EXPECT_EQ(completed, testIterations);
}

TEST_F(MutexTest, TestAsyncMixed)
{
    constexpr std::size_t testIterations = 10;
    std::size_t completed = 0;
    for (std::size_t i = 0; i < testIterations; i++)
    {
        ctx->spawn(
            testAsyncAddition() | sdbusplus::async::execution::then([&]() {
                const std::size_t done = ++completed;
                if (done == 2 * testIterations)
                {
                    ctx->request_stop();
                }
            }));
        ctx->spawn(
            testAsyncSubtraction(2) | sdbusplus::async::execution::then([&]() {
                const std::size_t done = ++completed;
                if (done == 2 * testIterations)
                {
                    ctx->request_stop();
                }
            }));
    }

    ctx->run();

    EXPECT_EQ(sharedVar, -static_cast<int>(testIterations));
    EXPECT_EQ(completed, 2 * testIterations);
}

TEST_F(MutexTest, TestFdEvents)
{
    constexpr static std::size_t testIterations = 5;

    std::atomic<std::size_t> completed = 0;
    for (std::size_t i = 0; i < testIterations; i++)
    {
        ctx->spawn(testFdEvents() | sdbusplus::async::execution::then([&]() {
                       const std::size_t done = ++completed;
                       if (done == testIterations)
                       {
                           ctx->request_stop();
                       }
                   }));
    }
    ctx->run();
    EXPECT_EQ(ran, testIterations);
    EXPECT_EQ(completed.load(), testIterations);
}

TEST_F(MutexTest, TestLockGuardNoLock)
{
    sdbusplus::async::lock_guard lg{mutex};
}
