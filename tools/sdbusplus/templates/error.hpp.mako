#pragma once

#include <sdbusplus/exception.hpp>

#include <cerrno>

namespace ${error.namespaces}
{
    % for e in error.errors:
struct ${e.CamelCase} final : public sdbusplus::exception::generated_exception
{
    static constexpr auto errName =
        "${error.name}.Error.${e.name}";
    static constexpr auto errDesc =
        "${e.description.strip()}";
    static constexpr auto errWhat =
        "${error.name}.Error.${e.name}: ${e.description.strip()}";
    % if e.errno:
    static constexpr auto errErrno = ${e.errno};
    % endif

    const char* name() const noexcept override
    {
        return errName;
    }
    const char* description() const noexcept override
    {
        return errDesc;
    }
    const char* what() const noexcept override
    {
        return errWhat;
    }
    % if e.errno:
    int get_errno() const noexcept override
    {
        return errErrno;
    }
    % endif
};
    % endfor
} // namespace ${error.namespaces}

#ifndef SDBUSPP_REMOVE_DEPRECATED_NAMESPACE
namespace ${error.old_namespaces}
{
    % for e in error.errors:
    using ${e.name} = ${error.namespaces}::${e.CamelCase};
    % endfor
} // namespace ${error.old_namespaces}
#endif\
