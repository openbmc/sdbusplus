#include <${ '/'.join(event.name.split('.')) }/event.hpp>
<% namespaces = event.name.split('.') %>
namespace sdbusplus
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
namespace Event
{
    % for e in event.events:
const char* ${e.name}::name() const noexcept
{
    return errName;
}
const char* ${e.name}::description() const noexcept
{
    return errDesc;
}
const char* ${e.name}::consumer() const noexcept
{
    return errConsumer;
}
    % endfor

} // namespace Event
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
