#pragma once

#include <exception>

namespace sdbusplus
{

namespace exception
{

struct exception : public std::exception
{
};

} // namespace exception

using exception_t = exception::exception;

} // namespace sdbusplus
