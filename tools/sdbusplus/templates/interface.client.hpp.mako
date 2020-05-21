#pragma once

<%
    namespaces = interface.name.split('.')
    classname = namespaces.pop()
%>
namespace sdbusplus
{
% for s in namespaces:
namespace ${s}
{
% endfor
namespace client
{
namespace ${classname}
{

static constexpr auto interface = "${interface.name}";

} // namespace ${classname}
} // namespace client
% for s in reversed(namespaces):
} // namespace ${s}
% endfor
} // namespace sdbusplus
