#include <signal.h>
#include <stdlib.h>
#include <string.h>

#include <sdbusplus/test/integration/daemon_manager.hpp>
#include <sdbusplus/test/integration/dbusd_manager.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace sdbusplus
{

namespace test
{

namespace integration
{

static inline const std::vector<std::string> initialArgv{
    "dbus-daemon", "--fork", "--print-address=1", "--print-pid=1"};

static std::vector<std::string> buildArgv(const std::string dbusConfPath,
                                          bool useSystemBus)
{
    std::vector<std::string> argv = initialArgv;
    if (useSystemBus)
    {
        argv.push_back("--config-file=" + dbusConfPath);
    }
    else
    {
        argv.push_back("--session");
    }

    return argv;
}

DBusDaemon::DBusDaemon(const std::string dbusConfPath,
                       const std::string stdoutPath, bool useSystemBus) :
    Daemon(buildArgv(dbusConfPath, useSystemBus), stdoutPath),
    useSystemBus(useSystemBus)
{}

DBusDaemon::~DBusDaemon()
{
    unsetBusAddressEnvVariable();
}

void DBusDaemon::postStart()
{
    std::vector<std::string> lines = captureStdoutLines();
    assert(lines.size() == 2);
    setForkedDaemonPid(lines[1].c_str());
    setBusAddressEnvVariable(lines[0].c_str());
}

std::string DBusDaemon::getPathNotFoundHelpMsg()
{
    return getExecutionPath() + R"( is required for running the test. "
        "It is available at https://github.com/openbmc/phosphor-objmgr."
        " Make sure the executable is available on system path.)";
}

void DBusDaemon::setForkedDaemonPid(const char* pidStr)
{
    pid_t pid = (int)strtol(pidStr, nullptr, 10);
    setPid(pid);
}

void DBusDaemon::setBusAddressEnvVariable(const char* address)
{
    if (useSystemBus)
    {
        setenv("DBUS_SYSTEM_BUS_ADDRESS", address, 1);
    }
    else
    {
        setenv("DBUS_SESSION_BUS_ADDRESS", address, 1);
    }
}

void DBusDaemon::unsetBusAddressEnvVariable()
{
    if (useSystemBus)
    {
        unsetenv("DBUS_SYSTEM_BUS_ADDRESS");
    }
    else
    {
        unsetenv("DBUS_SESSION_BUS_ADDRESS");
    }
}

} // namespace integration
} // namespace test
} // namespace sdbusplus
