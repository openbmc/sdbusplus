#include <sdbusplus/async/scope.hpp>

#include <exception>

namespace sdbusplus::async
{
scope::~scope() noexcept(false)
{
    if (count != 0)
    {
        throw std::logic_error(
            "sdbusplus::async::scope destructed while tasks are pending.");
    }
}

void scope::started_task() noexcept
{
    ++count;
}

void scope::ended_task() noexcept
{
    --count;
}

} // namespace sdbusplus::async
