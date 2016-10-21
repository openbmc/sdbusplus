from .property import Property
from .namedelement import NamedElement
from .renderer import Renderer


class Method(NamedElement, Renderer):
    def __init__(self, **kwargs):
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
