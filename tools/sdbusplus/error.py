import os
import yaml
from .namedelement import NamedElement
from .renderer import Renderer


class Error(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir='.'):
        filename = os.path.join(rootdir,
                                name.replace('.', '/') + ".errors.yaml")

        with open(filename) as f:
            data = f.read()
            y = yaml.safe_load(data)
            y = {'name': name,
                 'errors': y}
            return Error(**y)

    def __init__(self, **kwargs):
        self.errors = \
            [NamedElement(**n) for n in kwargs.pop('errors', [])]

        super(Error, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "error.md.mako", error=self)

    def exception_header(self, loader):
        return self.render(loader, "error.hpp.mako", error=self)

    def exception_cpp(self, loader):
        return self.render(loader, "error.cpp.mako", error=self)
