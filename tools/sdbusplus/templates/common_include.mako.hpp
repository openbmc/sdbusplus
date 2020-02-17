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
namespace ${classname}
{

static constexpr auto interface = "${interface.name}";

% for e in interface.enums:
namespace ${e.name}
{

    % for v in e.values:
static constexpr auto ${v.name} = "${interface.name}.${e.name}.${v.name}";
    % endfor

} // namespace ${e.name}
% endfor

} // namespace ${classname}
% for s in reversed(namespaces):
} // namespace ${s}
% endfor
} // namespace sdbusplus
