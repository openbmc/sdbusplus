import os

import yaml

from .enum import Enum
from .method import Method
from .namedelement import NamedElement
from .property import Property
from .renderer import Renderer
from .signal import Signal


class Interface(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir="."):
        filename = os.path.join(
            rootdir, name.replace(".", "/") + ".interface.yaml"
        )

        with open(filename) as f:
            data = f.read()
            y = yaml.safe_load(data)
            y["name"] = name
            return Interface(**y)

    def __init__(self, **kwargs):
        self.properties = [Property(**p) for p in kwargs.pop("properties", [])]
        self.methods = [Method(**m) for m in kwargs.pop("methods", [])]
        self.signals = [Signal(**s) for s in kwargs.pop("signals", [])]
        self.enums = [Enum(**e) for e in kwargs.pop("enumerations", [])]

        super(Interface, self).__init__(**kwargs)

    def joinedName(self, join_str, append):
        return join_str.join(self.namespaces + [self.classname, append])

    def error_includes(self, inc_list):
        includes = []
        for e in inc_list:
            e = e.replace("self.", self.name + ".")
            n = "/".join(
                e.split(".")[:-2],  # ignore the Error.Name
            )
            includes.append(f"{n}/error.hpp")
        return sorted(set(includes))

    def errorNamespacedClass(self, error):
        error = error.replace("self.", self.name + ".")
        return "sdbusplus::" + "::".join(error.split("."))

    def enum_includes(self, inc_list):
        includes = []
        for e in inc_list:
            includes.extend(e.enum_headers(self.name, self.typename))
        return sorted(set(includes))

    def markdown(self, loader):
        return self.render(loader, "interface.md.mako", interface=self)

    def server_header(self, loader):
        self.typename = "server"
        return self.render(loader, "interface.server.hpp.mako", interface=self)

    def server_cpp(self, loader):
        self.typename = "server"
        return self.render(loader, "interface.server.cpp.mako", interface=self)

    def client_header(self, loader):
        self.typename = "client"
        return self.render(loader, "interface.client.hpp.mako", interface=self)

    def common_header(self, loader):
        self.typename = "common"
        return self.render(loader, "interface.common.hpp.mako", interface=self)

    def cpp_includes(self):
        return sorted(
            set.union(
                set(),
                *[
                    set(m.cpp_includes(self))
                    for m in self.methods + self.properties + self.signals
                ],
            )
        )
