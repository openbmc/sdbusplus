#pragma once
#include <tuple>
#include <systemd/sd-bus.h>
#include <sdbusplus/vtable.hpp>
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

class ${classname}
{
    public:
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='header') }
    % endfor

    private:
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-header') }
    % endfor

        static const sdbusplus::vtable::vtable_t _vtable[];
};

    % for s in namespaces:
} // namespace ${s}
    % endfor
} // namespace server
} // namespace sdbusplus
