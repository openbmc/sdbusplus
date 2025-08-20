#pragma once

#include <exec/task.hpp>

// Add exec::task as sdbusplus::async::task so that we can simplify reference to
// any parts of it we use and as a portable alias that library users can
// reference.
namespace sdbusplus::async
{
template <typename T = void>
using task = exec::task<T>;
} // namespace sdbusplus::async
