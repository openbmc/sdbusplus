#pragma once

#include <exception>

namespace sdbusplus
{

namespace exception
{

struct exception : public std::exception
{
};

}

using exception_t = exception::exception;

}
