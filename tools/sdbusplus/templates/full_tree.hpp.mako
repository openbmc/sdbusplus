#pragma once

% for interface in interfaces:
#include <${interface.headerFile("common")}>
% endfor

% for sn in structNodes:
${structNodes[sn].render(loader, "structNode.hpp.mako", structNode=structNodes[sn])}
% endfor

// create constexprs for toplevel interface namespaces
% for sn in structNodes:
static inline constexpr ${structNodes[sn].getStructName()} ${structNodes[sn].nodeName} {};
% endfor
