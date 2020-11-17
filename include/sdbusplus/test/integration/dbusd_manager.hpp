#pragma once

#include <sdbusplus/test/integration/daemon_manager.hpp>

#include <string>

namespace sdbusplus
{

namespace test
{

namespace integration
{

class DBusDaemon : public Daemon
{
  public:
    DBusDaemon(const DBusDaemon&) = delete;
    DBusDaemon& operator=(const DBusDaemon&) = delete;
    DBusDaemon(DBusDaemon&&) = delete;
    DBusDaemon& operator=(DBusDaemon&&) = delete;

    DBusDaemon(const std::string dbusConfPath, const std::string stdoutPath,
               bool useSystemBus = true);

    ~DBusDaemon();

  protected:
    void postStart() override;

    std::string getPathNotFoundHelpMsg() override;

  private:
    void setForkedDaemonPid(const char* pidStr);

    void setBusAddressEnvVariable(const char* address);

    void unsetBusAddressEnvVariable();

    bool useSystemBus;
};

} // namespace integration
} // namespace test
} // namespace sdbusplus
