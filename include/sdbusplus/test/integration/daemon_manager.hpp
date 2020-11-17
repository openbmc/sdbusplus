#pragma once

#include <spawn.h>  // for posix_spawn
#include <unistd.h> //for pid_t

#include <string>
#include <string_view>
#include <vector>

namespace sdbusplus
{

namespace test
{

namespace integration
{

class Daemon
{
  public:
    Daemon() = delete;
    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;
    Daemon(Daemon&&) = delete;
    Daemon& operator=(Daemon&&) = delete;

    Daemon(std::vector<std::string> argvList, const std::string stdoutPath = "",
           const std::string stderrPath = "");

    void start(int warmUpMilisec = 1000);

    bool isStarted();

    std::vector<std::string> captureStdoutLines();

    virtual ~Daemon() noexcept(false);

  protected:
    virtual void postStart();

    std::string getExecutionPath();

    void setPid(pid_t in);

    pid_t getPid();

    void throwDaemonError(const std::string& msgBeforeCMD);

    virtual std::string getPathNotFoundHelpMsg();

  private:
    void spawnProcess();

    void terminate();

    bool isDaemonTerminated();

    void forceKillDaemon();

    void buildCmd();

    bool haveFileActions();

    posix_spawn_file_actions_t*
        buildPosixSpawnFileActions(posix_spawn_file_actions_t* action);
    void destroyPosixSpawnFileActions(posix_spawn_file_actions_t* action);

    const std::string stdoutPath;
    const std::string stderrPath;

    pid_t pid;
    int status;
    bool started;
    std::string cmd;
    std::vector<std::string> argvStr;

    static const int captureLineLimits = 100;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
