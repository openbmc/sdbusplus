#pragma once

// The upstream code has some warnings under GCC, so turn them off
// as needed.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <sdbusplus/async/std_execution/coroutine.hpp>
#include <sdbusplus/async/std_execution/execution.hpp>
#pragma GCC diagnostic pop

// Add std::this_thread::sync_wait from _P2300 as this is currently missing
// upstream.
namespace std::this_thread
{
using _P2300::this_thread::sync_wait;
}

// Add std::execution as sdbusplus::async::execution so that we can simplify
// reference to any parts of it we use internally.
namespace sdbusplus::async
{
namespace execution = std::execution;
}
