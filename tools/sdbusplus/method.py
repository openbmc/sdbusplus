from .property import Property
from .namedelement import NamedElement
from .renderer import Renderer


class Method(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.flags = kwargs.pop('flags', [])
        self.cppFlags = self.cpp_flags(self.flags)
        self.parameters = \
            [Property(**p) for p in kwargs.pop('parameters', [])]
        self.returns = \
            [Property(**r) for r in kwargs.pop('returns', [])]
        self.errors = kwargs.pop('errors', [])

        super(Method, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "method.mako.md", method=self)

    def cpp_prototype(self, loader, interface, ptype):
        return self.render(loader, "method.mako.prototype.hpp", method=self,
                           interface=interface, ptype=ptype, post=str.rstrip)

    def cpp_flags(self, flags):
        flags_map = {
            "deprecated":"vtable::common_::deprecated",
            "hidden":"vtable::common_::hidden",
            "unprivileged":"vtable::common_::unprivileged",
            "no_reply":"vtable::method_::no_reply"
        }

        l = []
        for flag in flags:
            r = flags_map.get(flag)
            if r is None:
                raise RuntimeError("Invalid method flag %s" % flag)

            l.append(r)

        return ' | '.join(l)
