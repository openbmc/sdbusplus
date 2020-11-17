#include <fcntl.h> // file control flags
#include <spawn.h> // for posix_spawn
#include <stdlib.h>
#include <sys/wait.h> // for wait_pid
#include <unistd.h>   //for pid_t

#include <sdbusplus/test/integration/daemon_manager.hpp>

#include <cassert>
#include <chrono>  // std::chrono::seconds
#include <csignal> // for SIGTERM
#include <cstring> // for strerror
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept> // for std::runtime_error
#include <string>
#include <string_view>
#include <thread> // std::this_thread::sleep_for
#include <vector>

extern char** environ;

namespace sdbusplus
{

namespace test
{

namespace integration
{

Daemon::Daemon(std::vector<std::string> argvList, const std::string oPath,
               const std::string ePath) :
    stdoutPath(oPath),
    stderrPath(ePath), pid(-1), started(false), argvStr(argvList)
{
    buildCmd();
}

Daemon::~Daemon() noexcept(false)
{
    terminate();
}

static void warmUp(int warmUpMilisec)
{
    if (warmUpMilisec > 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(warmUpMilisec));
    }
}

void Daemon::start(int warmUpMilisec)
{
    spawnProcess();
    warmUp(warmUpMilisec);
    postStart();
    started = true;
}

bool Daemon::isStarted()
{
    return started;
}

std::vector<std::string> Daemon::captureStdoutLines()
{
    std::vector<std::string> outputList;
    if (!stdoutPath.empty())
    {
        std::ifstream dOutstream(stdoutPath);
        std::string line;
        int lineCount = 0;
        while (lineCount < captureLineLimits && std::getline(dOutstream, line))
        {
            outputList.push_back(line);
            lineCount++;
        }
        dOutstream.close();
    }
    return outputList;
}

void Daemon::buildCmd()
{
    for (size_t i = 0; i < argvStr.size(); i++)
    {
        cmd += argvStr[i];
        if (i < argvStr.size() - 1)
        {
            cmd += " ";
        }
    }
}

void Daemon::spawnProcess()
{
    posix_spawn_file_actions_t* action = nullptr;
    if (haveFileActions())
    {
        posix_spawn_file_actions_t defaultAction;
        action = buildPosixSpawnFileActions(&defaultAction);
    }

    std::vector<const char*> argv;
    for (const std::string& arg : argvStr)
    {
        argv.push_back(arg.c_str());
    }
    argv.push_back(nullptr);

    status = posix_spawnp(&pid, argv[0], action, nullptr,
                          const_cast<char* const*>(argv.data()), environ);
    if (action != nullptr)
    {
        destroyPosixSpawnFileActions(action);
    }
    if (status != 0)
    {
        throwDaemonError("Failed to start daemon with command: ");
    }
}

void Daemon::terminate()
{
    assert(pid > 0);
    kill(pid, SIGTERM);
    if (!isDaemonTerminated())
    {
        waitpid(pid, &status, 0);
        if (!isDaemonTerminated())
        {
            forceKillDaemon();
        }
    }
    started = false;
}

bool Daemon::isDaemonTerminated()
{
    return (getpgid(pid) == -1 && errno == ESRCH);
}

void Daemon::forceKillDaemon()
{
    int rc = kill(getPid(), SIGKILL);
    if (rc != 0)
    {
        throwDaemonError("Failed to kill daemon that started with command: ");
    }
}

posix_spawn_file_actions_t*
    Daemon::buildPosixSpawnFileActions(posix_spawn_file_actions_t* action)
{
    int rc = posix_spawn_file_actions_init(action);
    if (rc != 0)
    {
        throwDaemonError(R"(Failed to initiate file action for output of daemon
                         that started with command: )");
    }
    mode_t permissionMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int accessFlags = O_WRONLY | O_CREAT | O_TRUNC;
    if (!stdoutPath.empty())
    {
        rc = posix_spawn_file_actions_addopen(action, STDOUT_FILENO,
                                              stdoutPath.c_str(), accessFlags,
                                              permissionMode);
        if (rc != 0)
        {
            throwDaemonError(
                "Failed to open " + stdoutPath +
                " as stdout for the daemon that started with command: ");
        }
    }
    if (!stderrPath.empty())
    {
        if (!stdoutPath.empty() && stderrPath == stdoutPath)
        {
            rc = posix_spawn_file_actions_adddup2(action, STDOUT_FILENO,
                                                  STDERR_FILENO);
        }
        else
        {
            rc = posix_spawn_file_actions_addopen(action, STDERR_FILENO,
                                                  stderrPath.c_str(),
                                                  accessFlags, permissionMode);
        }
        if (rc != 0)
        {
            throwDaemonError(
                "Failed to open " + stderrPath +
                " as stderr for the daemon that started with command: ");
        }
    }
    return action;
}

void Daemon::destroyPosixSpawnFileActions(posix_spawn_file_actions_t* action)
{
    int rc = 0;
    if (!stdoutPath.empty())
    {
        rc = posix_spawn_file_actions_addclose(action, STDOUT_FILENO);
        if (rc != 0)
        {
            throwDaemonError(
                "Failed to close " + stdoutPath +
                " as stdout for the daemon that started with command: ");
        }
    }
    if (!stderrPath.empty())
    {
        rc = posix_spawn_file_actions_addclose(action, STDERR_FILENO);
        if (rc != 0)
        {
            throwDaemonError(
                "Failed to close " + stderrPath +
                " as stderr for the daemon that started with command: ");
        }
    }
    rc = posix_spawn_file_actions_destroy(action);
    if (rc != 0)
    {
        throwDaemonError(R"(Failed to destroy file action for output of daemon
                         that started with command: )");
    }
}

void Daemon::postStart()
{
    if (waitpid(pid, &status, WNOHANG) == -1)
    {
        throwDaemonError("Failed to start daemon with command: ");
    }
    if (status != 0)
    {
        throwDaemonError("Daemon started, but failed. Command: ");
    }
}

bool Daemon::haveFileActions()
{
    return !(stdoutPath.empty() && stderrPath.empty());
}

void Daemon::throwDaemonError(const std::string& msgBeforeCMD)
{
    std::stringstream ss;
    ss << msgBeforeCMD << cmd << std::endl
       << "Error no: " << errno << ", Error description: " << strerror(errno)
       << std::endl;
    if (errno == ENOENT)
    {
        ss << getPathNotFoundHelpMsg();
    }
    throw std::runtime_error(ss.str());
}

std::string Daemon::getPathNotFoundHelpMsg()
{
    std::stringstream ss;
    ss << "Make sure " << getExecutionPath() << " is available";
    return ss.str();
}

std::string Daemon::getExecutionPath()
{
    return argvStr[0];
}

void Daemon::setPid(pid_t in)
{
    pid = in;
}

pid_t Daemon::getPid()
{
    return pid;
}

} // namespace integration
} // namespace test
} // namespace sdbusplus
