#pragma once

#include <sdbusplus/test/integration/daemon_manager.hpp>

#include <string>

namespace sdbusplus
{

namespace test
{

namespace integration
{

/** This class inherits the Daemon class and is responsible for managing
 * dbus-daemon specifically.
 *
 * This class is used by the PrivateBus class and no other user is intended for
 * this class.
 */
class DBusDaemon : public Daemon
{
  public:
    DBusDaemon(const DBusDaemon&) = delete;
    DBusDaemon& operator=(const DBusDaemon&) = delete;
    DBusDaemon(DBusDaemon&&) = delete;
    DBusDaemon& operator=(DBusDaemon&&) = delete;

    /** Constructs a DBus Daemon manager object.
     *
     * @param dbusConfPath - the path to the dbus configuration file
     * @param stdoutPath - the path to the file to capture stdout
     * @param useSystemBus - a flag to determine whether the system bus should
     * be used or not.
     */
    DBusDaemon(const std::string dbusConfPath, const std::string stdoutPath,
               bool useSystemBus = true);

    ~DBusDaemon();

  protected:
    /** Customize the post start activity for this daemon type.
     *
     * It captures the correct pid of the forked child by dbus-daemon from the
     * daemon output and sets its value in this object.
     * Next, it sets the environment variables for the DBus address that will
     * be used by any processes run by this test.
     */
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
