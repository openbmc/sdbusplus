#pragma once

#include <systemd/sd-bus.h>

#include <sdbusplus/sdbus.hpp>

#include <exception>
#include <string>

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
    virtual int get_errno() const noexcept
    {
        return EIO;
    }
};

/** base exception class for all errors generated by sdbusplus itself. */
struct internal_exception : public exception
{};

/** Exception for when an underlying sd_bus method call fails. */
class SdBusError final : public internal_exception
{
  public:
    /** Errno must be positive */
    SdBusError(int error, const char* prefix,
               SdBusInterface* intf = &sdbus_impl);
    /** Becomes the owner of the error */
    SdBusError(sd_bus_error* error, const char* prefix,
               SdBusInterface* intf = &sdbus_impl);

    SdBusError(const SdBusError&) = delete;
    SdBusError& operator=(const SdBusError&) = delete;
    SdBusError(SdBusError&& other);
    SdBusError& operator=(SdBusError&& other);
    virtual ~SdBusError();

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
    int get_errno() const noexcept override;
    const sd_bus_error* get_error() const noexcept;

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

/** Exception for when unpackProperties cannot find given property in provided
 * container */
class UnpackPropertyError final : public internal_exception
{
  public:
    UnpackPropertyError(std::string_view propertyName, std::string_view reason);

    static constexpr std::string_view reasonMissingProperty =
        "Missing property";
    static constexpr std::string_view reasonTypeNotMatched = "Type not matched";

    static constexpr auto errName =
        "xyz.openbmc_project.sdbusplus.Error.UnpackPropertyError";
    static constexpr auto errDesc =
        "unpackProperties failed to unpack one of requested properties.";
    static constexpr auto errWhat =
        "xyz.openbmc_project.sdbusplus.Error.UnpackPropertyError: "
        "unpackProperties failed to unpack one of requested properties.";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;

    const std::string propertyName;
    const std::string reason;
};

} // namespace exception

using exception_t = exception::exception;
using internal_exception_t = exception::internal_exception;

} // namespace sdbusplus
