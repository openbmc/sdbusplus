#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <sdbusplus/async/stdexec/task.hpp>
#pragma GCC diagnostic pop

namespace sdbusplus::async
{
template <typename T = void>
using task = exec::task<T>;
}
