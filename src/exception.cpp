#include <sdbusplus/exception.hpp>
#include <sdbusplus/sdbuspp_support/event.hpp>

#include <cerrno>
#include <stdexcept>
#include <utility>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
#endif

namespace sdbusplus::exception
{

void exception::unused() const noexcept {}

int exception::set_error(sd_bus_error* e) const
{
    return sd_bus_error_set(e, name(), description());
}

int exception::set_error(SdBusInterface* i, sd_bus_error* e) const
{
    return i->sd_bus_error_set(e, name(), description());
}

int generated_exception::get_errno() const noexcept
{
    return EIO;
}

SdBusError::SdBusError(int error_in, const char* prefix,
                       SdBusInterface* intf_in) :
    SdBusError(error_in, std::string(prefix), intf_in)
{}

SdBusError::SdBusError(int error_in, std::string&& prefix,
                       SdBusInterface* intf_in) :
    error(SD_BUS_ERROR_NULL), intf(intf_in)
{
    // We can't check the output of intf->sd_bus_error_set_errno() because
    // it returns the input errorcode. We don't want to try and guess
    // possible error statuses. Instead, check to see if the error was
    // constructed to determine success.
    intf->sd_bus_error_set_errno(&this->error, error_in);
    if (!intf->sd_bus_error_is_set(&this->error))
    {
        throw std::runtime_error("Failed to create SdBusError");
    }

    populateMessage(std::move(prefix));
}

SdBusError::SdBusError(sd_bus_error* error_in, const char* prefix,
                       SdBusInterface* intf_in) :
    error(*error_in), intf(intf_in)
{
    // We own the error so remove the caller's reference
    *error_in = SD_BUS_ERROR_NULL;

    populateMessage(std::string(prefix));
}

SdBusError::SdBusError(SdBusError&& other) : error(SD_BUS_ERROR_NULL)
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

int SdBusError::get_errno() const noexcept
{
    return intf->sd_bus_error_get_errno(&this->error);
}

const sd_bus_error* SdBusError::get_error() const noexcept
{
    return &error;
}

void SdBusError::populateMessage(std::string&& prefix)
{
    full_message = std::move(prefix);
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

int InvalidEnumString::get_errno() const noexcept
{
    return EINVAL;
}

static std::string unpackErrorReasonToString(const UnpackErrorReason reason)
{
    switch (reason)
    {
        case UnpackErrorReason::missingProperty:
            return "Missing property";
        case UnpackErrorReason::wrongType:
            return "Type not matched";
    }
    return "Unknown";
}

UnpackPropertyError::UnpackPropertyError(std::string_view propertyNameIn,
                                         const UnpackErrorReason reasonIn) :
    propertyName(propertyNameIn), reason(reasonIn),
    errWhatDetailed(std::string(errWhat) + " PropertyName: '" + propertyName +
                    "', Reason: '" + unpackErrorReasonToString(reason) + "'.")
{}

const char* UnpackPropertyError::name() const noexcept
{
    return errName;
}

const char* UnpackPropertyError::description() const noexcept
{
    return errDesc;
}

const char* UnpackPropertyError::what() const noexcept
{
    return errWhatDetailed.c_str();
}

int UnpackPropertyError::get_errno() const noexcept
{
    return EINVAL;
}

const char* UnhandledStop::name() const noexcept
{
    return errName;
}

const char* UnhandledStop::description() const noexcept
{
    return errDesc;
}

const char* UnhandledStop::what() const noexcept
{
    return errWhat;
}

int UnhandledStop::get_errno() const noexcept
{
    return ECANCELED;
}

static std::unordered_map<std::string, sdbusplus::sdbuspp::register_hook>
    event_hooks = {};

void throw_via_json(const nlohmann::json& j, const std::source_location& source)
{
    for (const auto& i : j.items())
    {
        if (auto it = event_hooks.find(i.key()); it != event_hooks.end())
        {
            it->second(j, source);
        }
    }
}

auto known_events() -> std::vector<std::string>
{
    std::vector<std::string> result{};

    for (const auto& [key, _] : event_hooks)
    {
        result.emplace_back(key);
    }

    std::ranges::sort(result);

    return result;
}

} // namespace sdbusplus::exception

namespace sdbusplus::sdbuspp
{

void register_event(const std::string& event, register_hook throw_hook)
{
    sdbusplus::exception::event_hooks.emplace(event, throw_hook);
}

} // namespace sdbusplus::sdbuspp

#ifdef __clang__
#pragma clang diagnostic pop
#endif
