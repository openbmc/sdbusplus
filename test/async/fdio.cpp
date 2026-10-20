#include <fcntl.h>
#include <unistd.h>

#include <sdbusplus/async.hpp>

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

using namespace std::literals;

namespace fs = std::filesystem;

struct FdioProbeCompletion : sdbusplus::async::fdio_ns::fdio_completion
{
    FdioProbeCompletion(sdbusplus::async::fdio& instance,
                        std::atomic_bool& completed,
                        std::atomic_bool& timedOut) :
        fdio_completion(instance), completed(completed), timedOut(timedOut)
    {}

  private:
    void complete() noexcept override
    {
        completed = true;
    }

    void error(std::exception_ptr) noexcept override
    {
        timedOut = true;
    }

    void stop() noexcept override {}

    std::atomic_bool& completed;
    std::atomic_bool& timedOut;
};

class FdioTest : public ::testing::Test
{
  protected:
    fs::path path;
    constexpr static auto testIterations = 5;

    FdioTest() : ctx(std::make_unique<sdbusplus::async::context>())
    {
        std::string pathTemplate =
            (fs::temp_directory_path() / "sdbusplus-fdio-XXXXXX").string();
        auto* tempPath = mkdtemp(pathTemplate.data());
        EXPECT_NE(tempPath, nullptr);
        if (tempPath != nullptr)
        {
            path = tempPath;
        }

        fd = inotify_init1(IN_NONBLOCK);
        EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                          << errno;

        wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
        EXPECT_NE(wd, -1)
            << "Error occurred during the inotify_add_watch, error: " << errno;
        fdioInstance = std::make_unique<sdbusplus::async::fdio>(*ctx, fd);
    }

    ~FdioTest() noexcept override
    {
        fdioInstance.reset();
        if (fd != -1)
        {
            if (wd != -1)
            {
                inotify_rm_watch(fd, wd);
            }
            close(fd);
        }
        if (!path.empty())
        {
            fs::remove_all(path);
        }
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

    auto testTimeoutChanges(bool& ran, std::chrono::microseconds& firstWait,
                            std::chrono::microseconds& secondWait)
        -> sdbusplus::async::task<>
    {
        using clock = std::chrono::steady_clock;
        auto start = clock::now();
        try
        {
            co_await fdioInstance->next();
            ADD_FAILURE() << "Expected the first wait to time out";
        }
        catch (const sdbusplus::async::fdio_timeout_exception&)
        {}
        firstWait = std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now() - start);

        start = clock::now();
        try
        {
            co_await fdioInstance->next();
            ADD_FAILURE() << "Expected the second wait to time out";
        }
        catch (const sdbusplus::async::fdio_timeout_exception&)
        {}
        secondWait = std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now() - start);
        ran = true;
        ctx->request_stop();
        co_return;
    }

    auto changeTimeout() -> sdbusplus::async::task<>
    {
        using namespace std::chrono_literals;
        co_await sdbusplus::async::sleep_for(*ctx, 20ms);
        fdioInstance->setTimeout(20ms);
        co_return;
    }

    auto stopAfter(std::chrono::milliseconds duration)
        -> sdbusplus::async::task<>
    {
        co_await sdbusplus::async::sleep_for(*ctx, duration);
        ctx->request_stop();
        co_return;
    }

    auto writePipeAfter(int fd, std::chrono::milliseconds delay)
        -> sdbusplus::async::task<>
    {
        co_await sdbusplus::async::sleep_for(*ctx, delay);
        const char byte = 'x';
        EXPECT_EQ(write(fd, &byte, sizeof(byte)), sizeof(byte));
        co_return;
    }

    auto toggleTimeouts(sdbusplus::async::fdio& watcher, int readFd,
                        int writeFd, bool& firstTimedOut,
                        bool& zeroWaitCompleted,
                        std::chrono::microseconds& zeroWait)
        -> sdbusplus::async::task<>
    {
        using clock = std::chrono::steady_clock;
        try
        {
            co_await watcher.next();
        }
        catch (const sdbusplus::async::fdio_timeout_exception&)
        {
            firstTimedOut = true;
        }

        watcher.setTimeout(0us);
        auto start = clock::now();
        ctx->spawn(writePipeAfter(writeFd, 120ms));
        try
        {
            co_await watcher.next();
            zeroWaitCompleted = true;
        }
        catch (const sdbusplus::async::fdio_timeout_exception&)
        {
            ADD_FAILURE() << "A zero timeout should disable the timer";
        }
        zeroWait = std::chrono::duration_cast<std::chrono::microseconds>(
            clock::now() - start);
        char byte;
        EXPECT_EQ(read(readFd, &byte, sizeof(byte)), sizeof(byte));

        watcher.setTimeout(30ms);
        try
        {
            co_await watcher.next();
            ADD_FAILURE() << "The re-enabled timeout should expire";
        }
        catch (const sdbusplus::async::fdio_timeout_exception&)
        {}
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

TEST_F(FdioTest, TimeoutChangesApplyToSubsequentNext)
{
    using namespace std::chrono_literals;
    fdioInstance->setTimeout(150ms);
    bool ran = false;
    std::chrono::microseconds firstWait{};
    std::chrono::microseconds secondWait{};
    ctx->spawn(testTimeoutChanges(ran, firstWait, secondWait));
    ctx->spawn(changeTimeout());
    ctx->run();

    EXPECT_TRUE(ran);
    EXPECT_GE(firstWait, 100ms);
    EXPECT_LT(firstWait, 500ms);
    EXPECT_GE(secondWait, 10ms);
    EXPECT_LT(secondWait, 100ms);
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

TEST_F(FdioTest, TimeoutCanBeDisabledAndReenabled)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(
        *ctx, fds[0], std::chrono::milliseconds(40));
    bool firstTimedOut = false;
    bool zeroWaitCompleted = false;
    std::chrono::microseconds zeroWait{};
    ctx->spawn(toggleTimeouts(*pipeFdio, fds[0], fds[1], firstTimedOut,
                              zeroWaitCompleted, zeroWait));
    ctx->spawn(stopAfter(500ms));
    ctx->run();

    EXPECT_TRUE(firstTimedOut);
    EXPECT_TRUE(zeroWaitCompleted);
    EXPECT_GE(zeroWait, 100ms);
    EXPECT_LT(zeroWait, 300ms);
    pipeFdio.reset();
    close(fds[0]);
    close(fds[1]);
}

TEST_F(FdioTest, CanceledWaitCannotTimeoutItsSuccessor)
{
    using namespace std::chrono_literals;
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_NONBLOCK), 0);
    auto pipeFdio = std::make_unique<sdbusplus::async::fdio>(*ctx, fds[0]);
    std::atomic_bool predecessorCompleted = false;
    std::atomic_bool predecessorTimedOut = false;
    std::atomic_bool successorCompleted = false;
    std::atomic_bool successorTimedOut = false;
    pipeFdio->setTimeout(100ms);
    {
        FdioProbeCompletion predecessor(*pipeFdio, predecessorCompleted,
                                        predecessorTimedOut);
        predecessor.start();
        std::this_thread::sleep_for(20ms);
    }

    pipeFdio->setTimeout(0us);
    auto start = std::chrono::steady_clock::now();
    std::chrono::steady_clock::duration elapsed{};
    {
        FdioProbeCompletion successor(*pipeFdio, successorCompleted,
                                      successorTimedOut);
        successor.start();
        ctx->spawn(writePipeAfter(fds[1], 140ms));
        ctx->spawn(stopAfter(250ms));
        ctx->run();
        elapsed = std::chrono::steady_clock::now() - start;
    }

    EXPECT_FALSE(predecessorCompleted);
    EXPECT_FALSE(predecessorTimedOut);
    EXPECT_TRUE(successorCompleted);
    EXPECT_FALSE(successorTimedOut);
    EXPECT_GE(elapsed, 140ms);
    EXPECT_LT(elapsed, 500ms);
    pipeFdio.reset();
    close(fds[0]);
    close(fds[1]);
}
