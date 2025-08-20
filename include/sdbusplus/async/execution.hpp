#pragma once

#include <exec/async_scope.hpp>
#include <stdexec/coroutine.hpp>
#include <stdexec/execution.hpp>

// Add std::execution as sdbusplus::async::execution so that we can simplify
// reference to any parts of it we use.
namespace sdbusplus::async
{
namespace execution = stdexec;
using async_scope = exec::async_scope;
} // namespace sdbusplus::async
