#pragma once

// The upstream code has some warnings under GCC, so turn them off
// as needed.

#pragma GCC diagnostic push
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnon-pod-varargs"
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wdeprecated-copy"
#else
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
#pragma GCC diagnostic ignored "-Wempty-body"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <sdbusplus/async/stdexec/async_scope.hpp>
#include <sdbusplus/async/stdexec/coroutine.hpp>
#include <sdbusplus/async/stdexec/execution.hpp>
#pragma GCC diagnostic pop

// Add std::execution as sdbusplus::async::execution so that we can simplify
// reference to any parts of it we use internally.
namespace sdbusplus::async
{
namespace execution = stdexec;
using async_scope = exec::async_scope;
} // namespace sdbusplus::async
