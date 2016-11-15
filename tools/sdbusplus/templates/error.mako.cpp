#include <${ '/'.join(error.name.split('.')) }/error.hpp>
<% namespaces = error.name.split('.') %>
namespace sdbusplus
{
    % for s in namespaces:
namespace ${s}
{
    % endfor
namespace Error
{
    % for e in error.errors:
const char* ${e.name}::name() const noexcept
{
    return errName;
}
const char* ${e.name}::description() const noexcept
{
    return errDesc;
}
const char* ${e.name}::what() const noexcept
{
    return errWhat;
}
    % endfor

} // namespace Error
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
