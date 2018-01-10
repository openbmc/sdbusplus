import os
import yaml
from .namedelement import NamedElement
from .property import Property
from .method import Method
from .signal import Signal
from .enum import Enum
from .renderer import Renderer


class Interface(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir='.'):
        filename = os.path.join(rootdir,
                                name.replace('.', '/') + ".interface.yaml")

        with open(filename) as f:
            data = f.read()
            y = yaml.safe_load(data)
            y['name'] = name
            return Interface(**y)

    def __init__(self, **kwargs):
        self.properties = \
            [Property(**p) for p in kwargs.pop('properties', [])]
        self.methods = \
            [Method(**m) for m in kwargs.pop('methods', [])]
        self.signals = \
            [Signal(**s) for s in kwargs.pop('signals', [])]
        self.enums = \
            [Enum(**e) for e in kwargs.pop('enumerations', [])]

        super(Interface, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "interface.mako.md", interface=self)

    def server_header(self, loader):
        return self.render(loader, "interface.mako.server.hpp.in", interface=self)

    def server_cpp(self, loader):
        return self.render(loader, "interface.mako.server.cpp.in", interface=self)
