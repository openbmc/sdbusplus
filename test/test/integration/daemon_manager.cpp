#include <sdbusplus/test/integration/daemon_manager.hpp>

#include <chrono>
#include <exception>
#include <thread>

#include <gtest/gtest.h>

using namespace std::literals::chrono_literals;
static const auto lettingProcessDie = 100ms;

using sdbusplus::test::integration::Daemon;

class DaemonTest : public ::testing::Test
{
  public:
    bool isProcessRunning(const char* cmd)
    {
        int rc = system((std::string("ps -C ") + std::string(cmd)).c_str());
        return rc == 0;
    };
};

TEST_F(DaemonTest, MultiArguments)
{
    {
        EXPECT_FALSE(isProcessRunning("watch"));
        Daemon d({"watch", "uptime", "-c", "-e"});
        d.start(100);
        EXPECT_TRUE(d.isStarted());
        EXPECT_TRUE(isProcessRunning("watch"));
    }
    std::this_thread::sleep_for(lettingProcessDie);
    EXPECT_FALSE(isProcessRunning("watch"));
}

TEST_F(DaemonTest, CaptureStdout)
{
    {
        EXPECT_FALSE(isProcessRunning("top"));
        Daemon d({"top"}, "topout.txt");
        d.start(100);
        EXPECT_TRUE(d.isStarted());
        EXPECT_TRUE(isProcessRunning("top"));
        auto lines = d.captureStdoutLines();
        EXPECT_GE(lines.size(), 1);
    }
    std::this_thread::sleep_for(lettingProcessDie);
    EXPECT_FALSE(isProcessRunning("top"));
}
