#include "sdbusplus/server/transaction.hpp"

namespace sdbusplus
{
namespace server
{
namespace transaction
{
namespace details
{

// Transaction Id
thread_local uint64_t id = 0;

} // namespace details
} // namespace transaction
} // namespace server
} // namespace sdbusplus
