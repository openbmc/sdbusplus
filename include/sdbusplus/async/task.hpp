#pragma once

#include <sdbusplus/async/stdexec/task.hpp>

namespace sdbusplus::async
{
template <typename T = void>
using task = exec::task<T>;
}
