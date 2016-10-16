#include <sdbusplus/bus.hpp>
#include <sdbusplus/message.hpp>
#include <${"/".join(interface.name.split('.') + [ 'server.hpp' ])}>
    <%
        namespaces = interface.name.split('.')
        classname = namespaces.pop()
    %>
namespace sdbusplus
{
namespace server
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

    % for s in namespaces:
} // namespace ${s}
    % endfor
} // namespace server
} // namespace sdbusplus
