/* Events for ${events.name}
 * Version: ${events.version}
 */
#pragma once
#include <sdbusplus/exception.hpp>
#include <sdbusplus/utility/consteval_string.hpp>

#include <cerrno>

% for h in events.cpp_includes():
#include <${h}>
% endfor
%if events.errors:

namespace sdbusplus::error::${events.cppNamespacedClass()}
{
% for e in events.errors:

${events.render(loader, "event.hpp.mako", events=events, event=e)}\
% endfor

} // namespace sdbusplus::error::${events.cppNamespacedClass()}
%endif
%if events.errors:

namespace sdbusplus::event::${events.cppNamespacedClass()}
{
% for e in events.events:

${events.render(loader, "event.hpp.mako", events=events, event=e)}\
% endfor

} // namespace sdbusplus::event::${events.cppNamespacedClass()}
%endif
