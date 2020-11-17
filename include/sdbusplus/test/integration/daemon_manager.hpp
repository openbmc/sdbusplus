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

/**
 * This is the base class for managing the resources associated with daemons
 * that we run externally. Examples of these daemons include: D-Bus daemon,
 * mapperx daemon, and daemons under test such as swampd and ipmid.
 * This class uses posix_spawn to start external processes.
 */
class Daemon
{
  public:
    Daemon() = delete;
    Daemon(const Daemon&) = delete;
    Daemon& operator=(const Daemon&) = delete;
    Daemon(Daemon&&) = delete;
    Daemon& operator=(Daemon&&) = delete;

    /** Constructs a daemon manager
     *
     * Note that the daemon does not start upon construction. To start a daemon,
     * @see start().
     *
     * @param argvList - The collection of strings that is passed to
     * spawn_process as the argv to the external service. The first element of
     * argv is the path to the executable and the rest of the elements are the
     * arguments that are passed to the process.
     * @param stdoutPath - Optional parameter, which is a path to a file.
     * Passing a filename indicates that the contents of the standard output
     * for this daemon should be captured.
     * @param stderrPath - Similar to the stdoutPath parameter for standard
     * error.
     */
    Daemon(std::vector<std::string> argvList, const std::string stdoutPath = "",
           const std::string stderrPath = "");

    /** Starts the daemon specifed in construction.
     *
     * @param warmUpMilisec - The amount of time in milliseconds to wait for
     * the daemon to start.
     */
    void start(int warmUpMilisec = 1000);

    /** A query to check whether the daemon is started properly.
     */
    bool isStarted();

    /** Captures the lines from the standard output if it was specified in
     * constructor.
     * @return the lines.
     * @see captureLineLimits for limits on the number of lines.
     */
    std::vector<std::string> captureStdoutLines();

    /** Terminates the running daemon.
     */
    virtual ~Daemon() noexcept(false);

  protected:
    /** Allow the derived classes to customize the post start activity for
     * daemons.
     * @see DbusdManager for an example.
     */
    virtual void postStart();

    /**
     * @return the execution path for this daemon based on the passed argv
     * during construction.
     */
    std::string getExecutionPath();

    /** Allow the derived classes to set the daemon pid.
     *
     * Some daemons such as dbus-daemon, need to fork their own child, so the
     * pid returned by spawn_process is not the pid of the target process.
     * The derived class is responsible for setting the actual pid in this
     * situation.
     * @see DbusdManager for an example.
     * @param in - the new pid to set.
     */
    void setPid(pid_t in);

    pid_t getPid();

    /** A common helper method to throw a runtime exception in case of errors.
     *
     * @param msgBeforeCMD - The customized initial part of the message for
     * the exception.
     */
    void throwDaemonError(const std::string& msgBeforeCMD);

    /** Allow the derived classes provide a help message to the user in case
     * the path to start the daemon is not found.
     *
     * @return The customized help message to print.
     */
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
