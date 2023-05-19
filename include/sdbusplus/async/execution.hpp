#pragma once

// The upstream code has some warnings under GCC, so turn them off
// as needed.

#pragma GCC diagnostic push
#ifndef __clang__
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#include <sdbusplus/async/stdexec/coroutine.hpp>
#include <sdbusplus/async/stdexec/execution.hpp>
#pragma GCC diagnostic pop

// Add std::execution as sdbusplus::async::execution so that we can simplify
// reference to any parts of it we use internally.
namespace sdbusplus::async
{
namespace execution = stdexec;
}
