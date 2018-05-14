#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace exception
{

SdBusError::SdBusError(int error, const char* description) :
    std::system_error(error, std::generic_category()), error(SD_BUS_ERROR_NULL)
{
    if (error == ENOMEM ||
        sd_bus_error_set_errno(&this->error, error) == -ENOMEM)
    {
        throw std::bad_alloc();
    }

    populateMessage(description);
}

SdBusError::SdBusError(sd_bus_error error, const char* description) :
    std::system_error(sd_bus_error_get_errno(&error), std::generic_category()),
    error(error)
{
    populateMessage(description);
}

SdBusError::~SdBusError()
{
    sd_bus_error_free(&error);
}

const char* SdBusError::name() const noexcept
{
    return error.name;
}

const char* SdBusError::description() const noexcept
{
    return description_internal;
}

const char* SdBusError::what() const noexcept
{
    return full_message.c_str();
}

void SdBusError::populateMessage(const char *description)
{
    full_message = error.name;
    full_message += ": ";
    const size_t description_start = full_message.length();
    full_message += description;
    description_internal = full_message.c_str() + description_start;
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
