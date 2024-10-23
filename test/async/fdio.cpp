#include <sdbusplus/async.hpp>

#include <filesystem>
#include <fstream>

#include <gtest/gtest.h>

namespace fs = std::filesystem;

fs::path path = "/tmp";

auto WriteToFile() -> sdbusplus::async::task<>
{
    std::ofstream outfile(std::string(path.c_str()) + "/test_fdio.txt");
    EXPECT_TRUE(outfile.is_open())
        << "Error occurred during file open, error: " << errno;
    outfile << "Test fdio!" << std::endl;
    outfile.close();

    co_return;
}

auto TestFSEvents(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
{
    auto fd = inotify_init1(IN_NONBLOCK);
    EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                      << errno;

    EXPECT_TRUE(fs::is_directory(path))
        << "Watch directory doesn't exist, directory: " << path;

    auto wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
    EXPECT_NE(wd, -1) << "Error occurred during the inotify_add_watch, error: "
                      << errno;

    // Schedule to write to /tmp/example.txt to trigger the event
    ctx.spawn(WriteToFile());

    // Wait for the event
    co_await sdbusplus::async::async_fdio(ctx, fd);

    co_return;
}

TEST(Fdio, TestEvents)
{
    sdbusplus::async::context ctx;
    ctx.spawn(TestFSEvents(ctx));
    ctx.spawn(
        sdbusplus::async::execution::just() |
        sdbusplus::async::execution::then([&ctx]() { ctx.request_stop(); }));
    ctx.run();
}
