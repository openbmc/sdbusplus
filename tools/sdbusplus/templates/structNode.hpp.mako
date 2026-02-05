% for member in structNode.members:
${structNode.members[member].render(loader, "structNode.hpp.mako", structNode=structNode.members[member])}
% endfor

% if structNode.hasInterface:
struct ${structNode.getStructName()} : public sdbusplus::common::${structNode.interface.cppNamespace()}::${structNode.interface.classname} {
% else:
struct ${structNode.getStructName()} {
% endif

% for member in structNode.members:
    static inline constexpr ${structNode.members[member].getStructName()} ${structNode.members[member].nodeName} {};
% endfor
};
