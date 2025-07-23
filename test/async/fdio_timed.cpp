#include <sdbusplus/async.hpp>

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

using namespace std::literals;

namespace fs = std::filesystem;

class FdioTimedTest : public ::testing::Test
{
  protected:
    enum class testWriteOperation
    {
        writeSync,
        writeAsync,
        writeSkip
    };

    const fs::path path = "/tmp/test_fdio_timed";

    FdioTimedTest()
    {
        if (!fs::exists(path))
        {
            fs::create_directory(path);
        }

        fd = inotify_init1(IN_NONBLOCK);
        EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                          << errno;

        wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
        EXPECT_NE(wd, -1)
            << "Error occurred during the inotify_add_watch, error: " << errno;
        fdioInstance = std::make_unique<sdbusplus::async::fdio>(
            *ctx, fd, std::chrono::microseconds(1000));
    }

    ~FdioTimedTest() noexcept override
    {
        if (fd != -1)
        {
            if (wd != -1)
            {
                inotify_rm_watch(fd, wd);
            }
            close(fd);
        }
        ctx.reset();

        if (fs::exists(path))
        {
            fs::remove_all(path);
        }
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

    auto testFdTimedEvents(bool& ran, testWriteOperation writeOperation,
                           int testIterations) -> sdbusplus::async::task<>
    {
        for (int i = 0; i < testIterations; i++)
        {
            switch (writeOperation)
            {
                case testWriteOperation::writeSync:
                    co_await writeToFile();
                    break;
                case testWriteOperation::writeAsync:
                    ctx->spawn(
                        sdbusplus::async::sleep_for(*ctx, 1s) |
                        stdexec::then([&]() { ctx->spawn(writeToFile()); }));
                    break;
                case testWriteOperation::writeSkip:
                default:
                    break;
            }

            bool receivedTimeout = false;

            try
            {
                co_await fdioInstance->next();
            }
            catch (const sdbusplus::async::fdio_timeout_exception& e)
            {
                receivedTimeout = true;
            }

            switch (writeOperation)
            {
                case testWriteOperation::writeSync:
                    EXPECT_FALSE(receivedTimeout) << "Expected event";
                    break;
                case testWriteOperation::writeAsync:
                case testWriteOperation::writeSkip:
                default:
                    EXPECT_TRUE(receivedTimeout) << "Expected timeout";
                    break;
            }
        }
        ran = true;

        co_return;
    }

    std::unique_ptr<sdbusplus::async::fdio> fdioInstance;
    std::optional<sdbusplus::async::context> ctx{std::in_place};

  private:
    int fd = -1;
    int wd = -1;
};

TEST_F(FdioTimedTest, TestWriteSkipWithTimeout)
{
    bool ran = false;
    ctx->spawn(testFdTimedEvents(ran, testWriteOperation::writeSkip, 1));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 2s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}

TEST_F(FdioTimedTest, TestWriteAsyncWithTimeout)
{
    bool ran = false;
    ctx->spawn(testFdTimedEvents(ran, testWriteOperation::writeAsync, 1));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 2s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}

TEST_F(FdioTimedTest, TestWriteAsyncWithTimeoutIterative)
{
    bool ran = false;
    ctx->spawn(testFdTimedEvents(ran, testWriteOperation::writeAsync, 100));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 2s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}

TEST_F(FdioTimedTest, TestWriteSync)
{
    bool ran = false;
    ctx->spawn(testFdTimedEvents(ran, testWriteOperation::writeSync, 1));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 2s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}

TEST_F(FdioTimedTest, TestWriteSyncIterative)
{
    bool ran = false;
    ctx->spawn(testFdTimedEvents(ran, testWriteOperation::writeSync, 100));
    ctx->spawn(
        sdbusplus::async::sleep_for(*ctx, 2s) |
        sdbusplus::async::execution::then([&]() { ctx->request_stop(); }));
    ctx->run();
    EXPECT_TRUE(ran);
}
