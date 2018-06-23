#pragma once

#include <exception>
#include <sdbusplus/sdbus.hpp>
#include <string>
#include <system_error>
#include <systemd/sd-bus.h>

extern sdbusplus::SdBusImpl sdbus_impl;

namespace sdbusplus
{

namespace exception
{

/** Base exception class for all sdbusplus exceptions, including those created
 *  by the bindings. */
struct exception : public std::exception
{
    virtual const char* name() const noexcept = 0;
    virtual const char* description() const noexcept = 0;
};

/** base exception class for all errors generated by sdbusplus itself. */
struct internal_exception : public exception
{
};

/** Exception for when an underlying sd_bus method call fails. */
class SdBusError final : public internal_exception, public std::system_error
{
  public:
    /** Errno must be positive */
    SdBusError(int error, const char* prefix,
               SdBusInterface* intf = &sdbus_impl);
    /** Becomes the owner of the error */
    SdBusError(sd_bus_error error, const char* prefix,
               SdBusInterface* intf = &sdbus_impl);

    SdBusError(const SdBusError&) = delete;
    SdBusError& operator=(const SdBusError&) = delete;
    SdBusError(SdBusError&& other);
    SdBusError& operator=(SdBusError&& other);
    virtual ~SdBusError();

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;

  private:
    sd_bus_error error;
    std::string full_message;
    SdBusInterface* intf;

    /** Populates the full_message from the stored
     *  error and the passed in prefix. */
    void populateMessage(const char* prefix);

    /** Helper to reduce duplicate move logic */
    void move(SdBusError&& other);
};

/** Exception for when an invalid conversion from string to enum is
 *  attempted. */
struct InvalidEnumString final : public internal_exception
{
    static constexpr auto errName =
        "xyz.openbmc_project.sdbusplus.Error.InvalidEnumString";
    static constexpr auto errDesc =
        "An enumeration mapping was attempted for which no valid enumeration "
        "value exists.";
    static constexpr auto errWhat =
        "xyz.openbmc_project.sdbusplus.Error.InvalidEnumString: "
        "An enumeration mapping was attempted for which no valid enumeration "
        "value exists.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

} // namespace exception

using exception_t = exception::exception;
using internal_exception_t = exception::internal_exception;

} // namespace sdbusplus
