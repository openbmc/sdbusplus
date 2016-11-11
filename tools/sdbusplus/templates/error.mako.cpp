#include <${ '/'.join(error.name.split('.')) }/error.hpp>
<% namespaces = error.name.split('.') %>
namespace sdbusplus
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
namespace common
{
    % for e in error.errors:
const char* ${e.name}::what() const noexcept
{
    return name;
}
    % endfor

} // namespace common
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
