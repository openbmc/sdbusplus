<% namespaces = error.name.split('.') %>\
#pragma once

#include <sdbusplus/exception.hpp>

#include <cerrno>

namespace sdbusplus::${"::".join(namespaces)}::Error
{
    % for e in error.errors:
struct ${e.name} final : public sdbusplus::exception::generated_exception
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

} // namespace sdbusplus::${"::".join(namespaces)}::Error\
