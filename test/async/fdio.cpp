#include <sdbusplus/async.hpp>

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

    FdioTest()
    {
        auto fd = inotify_init1(IN_NONBLOCK);
        EXPECT_NE(fd, -1) << "Error occurred during the inotify_init1, error: "
                          << errno;

        auto wd = inotify_add_watch(fd, path.c_str(), IN_CLOSE_WRITE);
        EXPECT_NE(wd, -1)
            << "Error occurred during the inotify_add_watch, error: " << errno;
        fdioInstance = std::make_unique<sdbusplus::async::fdio>(*ctx, fd);
    }

    ~FdioTest() override
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

    auto testFdEvents(bool& ran,
                      bool sleepBeforeWrite) -> sdbusplus::async::task<>
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

    std::unique_ptr<sdbusplus::async::fdio> fdioInstance;
    std::optional<sdbusplus::async::context> ctx{std::in_place};

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
