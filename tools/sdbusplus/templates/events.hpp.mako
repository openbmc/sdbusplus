/* Events for ${events.name}
 * Version: ${events.version}
 */
#pragma once
#include <pthread.h>
#include <sys/syslog.h>

#include <sdbusplus/exception.hpp>

#include <cerrno>
#include <source_location>

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
%if events.events:

namespace sdbusplus::event::${events.cppNamespacedClass()}
{
% for e in events.events:

${events.render(loader, "event.hpp.mako", events=events, event=e)}\
% endfor

} // namespace sdbusplus::event::${events.cppNamespacedClass()}
%endif
