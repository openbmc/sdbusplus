#pragma once

namespace sdbusplus
{
% for s in interface.namespaces:
namespace ${s}
{
% endfor
namespace client
{
namespace ${interface.classname}
{

static constexpr auto interface = "${interface.name}";

} // namespace ${interface.classname}
} // namespace client
% for s in reversed(interface.namespaces):
} // namespace ${s}
% endfor
} // namespace sdbusplus
