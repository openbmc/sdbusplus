import os

import yaml

from .namedelement import NamedElement
from .renderer import Renderer


class ErrorElement(NamedElement):
    def __init__(self, **kwargs):
        super(ErrorElement, self).__init__(**kwargs)
        self.errno = kwargs.pop("errno", False)


class Error(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir, schemadir):
        filename = os.path.join(
            rootdir, name.replace(".", "/") + ".errors.yaml"
        )

        with open(filename) as f:
            data = f.read()
            y = yaml.safe_load(data)
            y = {"name": name, "errors": y}
            return Error(**y)

    def __init__(self, **kwargs):
        self.errors = [ErrorElement(**n) for n in kwargs.pop("errors", [])]

        super(Error, self).__init__(**kwargs)

        self.old_namespaces = self.name.split(".")
        self.namespaces = [
            NamedElement(name=x).snake_case for x in self.old_namespaces
        ]
        self.old_namespaces = "::".join(
            ["sdbusplus"] + self.old_namespaces + ["Error"]
        )
        self.namespaces = "::".join(["sdbusplus", "error"] + self.namespaces)

    def markdown(self, loader):
        return self.render(loader, "error.md.mako", error=self)

    def exception_header(self, loader):
        return self.render(loader, "error.hpp.mako", error=self)

    def exception_cpp(self, loader):
        return self.render(loader, "error.cpp.mako", error=self)
