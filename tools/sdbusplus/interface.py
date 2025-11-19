import os

import yaml

from .association import Association
from .enum import Enum
from .method import Method
from .namedelement import NamedElement
from .path import Path
from .property import Property
from .renderer import Renderer
from .servicename import ServiceName
from .signal import Signal


class Interface(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir, schemadir):
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
        self.paths = [Path(**p) for p in kwargs.pop("paths", [])]
        self.associations = [
            Association(**a) for a in kwargs.pop("associations", [])
        ]
        self.service_names = [
            ServiceName(**s) for s in kwargs.pop("service_names", [])
        ]

        super(Interface, self).__init__(**kwargs)

    def joinedName(self, join_str, append):
        return join_str.join(self.namespaces + [self.classname, append])

    def error_includes(self, inc_list):
        includes = []
        for e in inc_list:
            e = e.replace("self.", self.name + ".")
            if ".Error." in e:
                n = "/".join(
                    e.split(".")[:-2],  # ignore the Error.Name at the end
                )
                includes.append(f"{n}/error.hpp")
            else:
                n = "/".join(
                    e.split(".")[:-1],  # ignore the .Name at the end
                )
                includes.append(f"{n}/event.hpp")
        return sorted(set(includes))

    def errorNamespacedClass(self, error):
        error = error.replace("self.", self.name + ".")
        if ".Error" not in error:
            error = "error." + error
        return "sdbusplus::" + "::".join(error.split("."))

    def enum_includes(self, inc_list):
        includes = []
        for e in inc_list:
            includes.extend(e.enum_headers())
        return sorted(set(includes))

    def markdown(self, loader):
        return self.render(loader, "interface.md.mako", interface=self)

    def async_server_header(self, loader):
        return self.render(
            loader, "interface.aserver.hpp.mako", interface=self
        )

    def server_header(self, loader):
        return self.render(loader, "interface.server.hpp.mako", interface=self)

    def server_cpp(self, loader):
        return self.render(loader, "interface.server.cpp.mako", interface=self)

    def client_header(self, loader):
        return self.render(loader, "interface.client.hpp.mako", interface=self)

    def common_header(self, loader):
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
