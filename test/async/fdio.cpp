#include <fcntl.h>
#include <unistd.h>

#include <sdbusplus/async.hpp>

#include <ctime>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

using namespace std::literals;

namespace fs = std::filesystem;

class FdioTest : public ::testing::Test
{
  protected:
    const fs::path path = "/tmp";
    constexpr static auto testIterations = 5;

    FdioTest() : ctx(std::make_unique<sdbusplus::async::context>())
    {
        auto fd = inotify_init1(IN_NONBLOCK);
        EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                          << errno;

        auto wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
        EXPECT_NE(wd, -1)
            << "Error occurred during the inotify_add_watch, error: " << errno;
        fdioInstance = std::make_unique<sdbusplus::async::fdio>(*ctx, fd);
    }

    ~FdioTest() noexcept override
    {
        if (fd != -1)
        {
            if (wd != -1)
            {
                inotify_rm_watch(fd, wd);
            }
            close(fd);
        }
        fdioInstance.reset();
        ctx.reset();
    }

    auto writeToFile() -> sdbusplus::async::task<>
    {
        std::ofstream outfile((path / "test_fdio.txt").native());
        EXPECT_TRUE(outfile.is_open())
            << "Error occurred during file open, error: " << errno;
        outfile << "Test fdio!" << std::endl;
        outfile.close();
        co_return;
    }

    auto testFdEvents(bool& ran, bool sleepBeforeWrite)
        -> sdbusplus::async::task<>
    {
        for (int i = 0; i < testIterations; i++)
        {
            if (sleepBeforeWrite)
            {
                ctx->spawn(sdbusplus::async::sleep_for(*ctx, 1s) |
                           stdexec::then([&]() { ctx->spawn(writeToFile()); }));
            }
            else
            {
                co_await writeToFile();
            }
            co_await fdioInstance->next();
        }
        ran = true;
        co_return;
    }

    auto stopAfter(std::chrono::milliseconds duration)
        -> sdbusplus::async::task<>
    {
        co_await sdbusplus::async::sleep_for(*ctx, duration);
        ctx->request_stop();
        co_return;
    }

    auto completeThenIdle(sdbusplus::async::fdio& watcher, bool& secondComplete,
                          std::chrono::microseconds& idleCpu,
                          std::chrono::microseconds& idleWall)
        -> sdbusplus::async::task<>
    {
        co_await watcher.next();
        const auto cpuStart = std::clock();
        const auto wallStart = std::chrono::steady_clock::now();
        co_await sdbusplus::async::sleep_for(*ctx, 50ms);
        idleCpu = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::duration<double>(
                static_cast<double>(std::clock() - cpuStart) / CLOCKS_PER_SEC));
        idleWall = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - wallStart);
        co_await watcher.next();
        secondComplete = true;
        ctx->request_stop();
        co_return;
    }

    auto failEachWait(sdbusplus::async::fdio& watcher, int& failures)
        -> sdbusplus::async::task<>
    {
        for (int i = 0; i < 2; i++)
        {
            try
            {
                co_await watcher.next();
                ADD_FAILURE() << "Expected the wait to report an error";
            }
            catch (const std::exception&)
            {
                failures++;
            }
        }
        ctx->request_stop();
        co_return;
    }

    std::unique_ptr<sdbusplus::async::context> ctx;
    std::unique_ptr<sdbusplus::async::fdio> fdioInstance;

  private:
    int fd = -1;
    int wd = -1;
};

TEST_F(FdioTest, TestFdEvents)
{
    bool ran = false;
    ctx->spawn(testFdEvents(ran, false));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 1s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}

TEST_F(FdioTest, TestFdEventsWithSleep)
{
    bool ran = false;
    ctx->spawn(testFdEvents(ran, true));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 5s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}

TEST_F(FdioTest, IdleReadableFdDoesNotSpin)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(*ctx, fds[0]);
    const char byte = 'x';
    ASSERT_EQ(write(fds[1], &byte, sizeof(byte)), sizeof(byte));

    auto wallStart = std::chrono::steady_clock::now();
    auto cpuStart = std::clock();
    ctx->spawn(stopAfter(150ms));
    ctx->run();
    auto cpuElapsed = std::clock() - cpuStart;
    auto wallElapsed = std::chrono::steady_clock::now() - wallStart;

    EXPECT_LT(std::chrono::duration<double>(
                  static_cast<double>(cpuElapsed) / CLOCKS_PER_SEC),
              std::chrono::duration<double>(wallElapsed) / 2);
    pipeFdio.reset();
    close(fds[0]);
    close(fds[1]);
}

TEST_F(FdioTest, IdleHangupDoesNotSpin)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(*ctx, fds[0]);
    close(fds[1]);

    auto wallStart = std::chrono::steady_clock::now();
    auto cpuStart = std::clock();
    ctx->spawn(stopAfter(150ms));
    ctx->run();
    auto cpuElapsed = std::clock() - cpuStart;
    auto wallElapsed = std::chrono::steady_clock::now() - wallStart;

    EXPECT_LT(std::chrono::duration<double>(
                  static_cast<double>(cpuElapsed) / CLOCKS_PER_SEC),
              std::chrono::duration<double>(wallElapsed) / 2);
    pipeFdio.reset();
    close(fds[0]);
}

TEST_F(FdioTest, ReadinessAfterCompletionIsIdleUntilNextWait)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(*ctx, fds[0]);
    const char byte = 'x';
    ASSERT_EQ(write(fds[1], &byte, sizeof(byte)), sizeof(byte));

    bool secondComplete = false;
    std::chrono::microseconds idleCpu{};
    std::chrono::microseconds idleWall{};
    ctx->spawn(completeThenIdle(*pipeFdio, secondComplete, idleCpu, idleWall));
    ctx->spawn(stopAfter(250ms));
    ctx->run();

    EXPECT_TRUE(secondComplete);
    EXPECT_GE(idleWall, 40ms);
    EXPECT_LT(idleWall, 500ms);
    EXPECT_LT(idleCpu, idleWall / 2);
    pipeFdio.reset();
    close(fds[0]);
    close(fds[1]);
}

TEST_F(FdioTest, HangupAfterCompletionIsIdleUntilNextWait)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(*ctx, fds[0]);
    close(fds[1]);

    bool secondComplete = false;
    std::chrono::microseconds idleCpu{};
    std::chrono::microseconds idleWall{};
    ctx->spawn(completeThenIdle(*pipeFdio, secondComplete, idleCpu, idleWall));
    ctx->spawn(stopAfter(250ms));
    ctx->run();

    EXPECT_TRUE(secondComplete);
    EXPECT_GE(idleWall, 40ms);
    EXPECT_LT(idleWall, 500ms);
    EXPECT_LT(idleCpu, idleWall / 2);
    pipeFdio.reset();
    close(fds[0]);
}

TEST_F(FdioTest, FailureToEnableIsReportedToEveryWait)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(*ctx, fds[0]);
    close(fds[0]);

    int failures = 0;
    ctx->spawn(failEachWait(*pipeFdio, failures));
    ctx->spawn(stopAfter(500ms));
    ctx->run();

    EXPECT_EQ(failures, 2);
    pipeFdio.reset();
    close(fds[1]);
}
