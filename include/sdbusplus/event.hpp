#pragma once

#include <string>

namespace sdbusplus
{

namespace events
{

struct events
{
    virtual ~events() = default;

    virtual const char* name() const noexcept = 0;
    virtual const char* description() const noexcept = 0;
    virtual const char* consumer() const noexcept = 0;
};

} // namespace events
} // namespace sdbusplus
