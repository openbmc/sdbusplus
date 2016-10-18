#include <sdbusplus/server.hpp>
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

${classname}::${classname}(bus::bus& bus, const char* path)
        : _${"_".join(interface.name.split('.'))}_interface(
                bus, path, _interface, _vtable, this)
{
}

    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='callback-cpp') }
    % endfor

const vtable::vtable_t ${classname}::_vtable[] = {
    vtable::start(),
    % for m in interface.methods:
${ m.cpp_prototype(loader, interface=interface, ptype='vtable') }
    % endfor
    vtable::end()
};

    % for s in namespaces:
} // namespace ${s}
    % endfor
} // namespace server
} // namespace sdbusplus
