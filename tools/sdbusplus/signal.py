from .property import Property
from .namedelement import NamedElement
from .renderer import Renderer

class Signal(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.properties = [ Property(**p) for p in
            kwargs.pop('properties', []) ]

        super(Signal, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "signal.mako.md", signal=self)
