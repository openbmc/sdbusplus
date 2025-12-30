#include "suite.hpp"

#include <unistd.h>

#include <sdbusplus/async.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <print>

#include <gtest/gtest.h>

using namespace std::literals;

namespace fs = std::filesystem;

FdioTimedTest::FdioTimedTest()
{
    constexpr auto path_base = "/tmp/test_fdio_timed";

    path = std::format("{}{}", path_base, getpid());

    if (!fs::exists(path))
    {
        fs::create_directory(path);
    }

    fd = inotify_init1(IN_NONBLOCK);
    EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                      << errno;

    wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
    EXPECT_NE(wd, -1) << "Error occurred during the inotify_add_watch, error: "
                      << errno;
    fdioInstance = std::make_unique<sdbusplus::async::fdio>(
        *ctx, fd, std::chrono::microseconds(1000));
}

FdioTimedTest::~FdioTimedTest() noexcept
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

auto FdioTimedTest::writeToFile() -> sdbusplus::async::task<>
{
    std::ofstream outfile((path / "test_fdio.txt").native());
    EXPECT_TRUE(outfile.is_open())
        << "Error occurred during file open, error: " << errno;
    outfile << "Test fdio!" << std::endl;
    outfile.close();
    co_return;
}

auto FdioTimedTest::testFdTimedEvents(
    bool& ran, testWriteOperation writeOperation, int testIterations)
    -> sdbusplus::async::task<>
{
    if (!ctx)
        co_return;
    auto& io = *ctx;

    for (int i = 0; i < testIterations; i++)
    {
        switch (writeOperation)
        {
            case testWriteOperation::writeSync:
                co_await writeToFile();
                break;
            case testWriteOperation::writeAsync:
                io.spawn(sdbusplus::async::sleep_for(io, 1s) |
                         stdexec::then([&]() { io.spawn(writeToFile()); }));
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
