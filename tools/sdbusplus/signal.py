from .namedelement import NamedElement
from .property import Property
from .renderer import Renderer


class Signal(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.properties = [Property(**p) for p in kwargs.pop("properties", [])]

        super(Signal, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "signal.md.mako", signal=self)

    def cpp_prototype(self, loader, interface, ptype):
        return self.render(
            loader,
            "signal.prototype.hpp.mako",
            signal=self,
            interface=interface,
            ptype=ptype,
            post=str.rstrip,
        )

    def cpp_includes(self, interface):
        return interface.enum_includes(self.properties)
