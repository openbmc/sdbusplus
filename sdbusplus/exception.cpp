#include <sdbusplus/exception.hpp>

namespace sdbusplus
{
namespace exception
{

const char *InvalidEnumString::name() const noexcept
{
    return errName;
}

const char *InvalidEnumString::description() const noexcept
{
    return errDesc;
}

const char *InvalidEnumString::what() const noexcept
{
    return errWhat;
}

} // namespace exception
} // namespace sdbusplus
