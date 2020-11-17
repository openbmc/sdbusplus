#include <stdlib.h> // for mkstemp

#include <sdbusplus/bus.hpp>
#include <sdbusplus/test/integration/dbusd_manager.hpp>
#include <sdbusplus/test/integration/private_bus.hpp>
#include <sdbusplus/test/integration/test_timer.hpp>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace sdbusplus
{

namespace test
{

namespace integration
{

static constexpr std::string_view confFileContent = R"(
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
    "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
    <type>system</type>
    <keep_umask/>
    <listen>unix:tmpdir=/tmp</listen>
    <standard_system_servicedirs />
    <policy context="default">
        <allow send_destination="*" eavesdrop="true"/>
        <allow eavesdrop="true"/>
        <allow own="*"/>
    </policy>
</busconfig>
)";

static std::string buildTempFile(char* tempFileTemplate)
{
    if (mkstemp(tempFileTemplate) == -1)
    {
        std::stringstream ss;
        ss << "failed to create temp file, Error no: " << errno
           << ", Error description: " << strerror(errno) << std::endl;
        throw std::runtime_error(ss.str());
    }
    return std::string(tempFileTemplate);
}

static std::string buildDbusDaemonStdoutFile()
{
    char fileNameTemplate[] = "/tmp/dbus_daemon_out_XXXXXX";
    return buildTempFile(fileNameTemplate);
}

static std::string buildDbusConfFile()
{
    char fileNameTemplate[] = "/tmp/pbusconfig_XXXXXX";
    auto confFileName = buildTempFile(fileNameTemplate);
    std::ofstream confStream(confFileName);
    confStream << confFileContent << std::flush;
    confStream.close();
    return confFileName;
}

PrivateBus::PrivateBus() :
    daemon(buildDbusConfFile(), buildDbusDaemonStdoutFile())
{
    /* This is set in the docker script, but we don't need it.*/
    unsetenv("DBUS_STARTER_BUS_TYPE");
    daemon.start(defaultWarmupMillis);
    bus = std::make_shared<sdbusplus::bus::bus>(sdbusplus::bus::new_default());
}

void PrivateBus::runFor(SdBusDuration microseconds)
{
    Timer test;
    auto duration = test.duration();
    while (duration < microseconds)
    {
        getBus()->process_discard(); // discard any unhandled messages
        getBus()->wait(microseconds - duration);
        duration = test.duration();
    }
}

std::shared_ptr<sdbusplus::bus::bus> PrivateBus::getBus()
{
    return bus;
}

} // namespace integration
} // namespace test
} // namespace sdbusplus
