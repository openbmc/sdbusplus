#pragma once

#include <sdbusplus/exception.hpp>
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
struct ${e.name} : public sdbusplus::exception_t
{
    static constexpr auto name = "${error.name}.${e.name}";
    static constexpr auto description =
            "${e.description.strip()}";

    const char* what() const noexcept override;
};

    % endfor
} // namespace common
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
