#pragma once

#include <sdbusplus/exception.hpp>
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
struct ${e.name} final : public sdbusplus::exception_t
{
    static constexpr auto errName = "${error.name}.Error.${e.name}";
    static constexpr auto errDesc =
            "${e.description.strip()}";
    static constexpr auto errWhat =
            "${error.name}.Error.${e.name}: ${e.description.strip()}";

    const char* name() const noexcept override;
    const char* description() const noexcept override;
    const char* what() const noexcept override;
};

    % endfor
} // namespace Error
    % for s in reversed(namespaces):
} // namespace ${s}
    % endfor
} // namespace sdbusplus
