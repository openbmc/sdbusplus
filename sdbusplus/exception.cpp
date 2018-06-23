#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbus.hpp>

extern sdbusplus::SdBusImpl sdbus_impl;

namespace sdbusplus
{
namespace exception
{

SdBusError::SdBusError(int error, const char* prefix, SdBusInterface* intf) :
    std::system_error(error, std::generic_category()), error(SD_BUS_ERROR_NULL),
    intf(intf)
{
    if (error == ENOMEM ||
        intf->sd_bus_error_set_errno(&this->error, error) == -ENOMEM)
    {
        throw std::bad_alloc();
    }

    populateMessage(prefix);
}

SdBusError::SdBusError(sd_bus_error* error, const char* prefix,
                       SdBusInterface* intf) :
    std::system_error(intf->sd_bus_error_get_errno(error),
                      std::generic_category()),
    error(*error), intf(intf)
{
    // We own the error so remove the caller's reference
    *error = SD_BUS_ERROR_NULL;

    populateMessage(prefix);
}

SdBusError::SdBusError(SdBusError&& other)
{
    move(std::move(other));
}

SdBusError& SdBusError::operator=(SdBusError&& other)
{
    if (this != &other)
    {
        move(std::move(other));
    }
    return *this;
}

SdBusError::~SdBusError()
{
    intf->sd_bus_error_free(&error);
}

const char* SdBusError::name() const noexcept
{
    return error.name;
}

const char* SdBusError::description() const noexcept
{
    return error.message;
}

const char* SdBusError::what() const noexcept
{
    return full_message.c_str();
}

void SdBusError::populateMessage(const char* prefix)
{
    full_message = prefix;
    if (error.name)
    {
        full_message += ": ";
        full_message += error.name;
    }
    if (error.message)
    {
        full_message += ": ";
        full_message += error.message;
    }
}

void SdBusError::move(SdBusError&& other)
{
    intf = std::move(other.intf);

    intf->sd_bus_error_free(&error);
    error = other.error;
    other.error = SD_BUS_ERROR_NULL;

    full_message = std::move(other.full_message);
}

const char* InvalidEnumString::name() const noexcept
{
    return errName;
}

const char* InvalidEnumString::description() const noexcept
{
    return errDesc;
}

const char* InvalidEnumString::what() const noexcept
{
    return errWhat;
}

} // namespace exception
} // namespace sdbusplus
