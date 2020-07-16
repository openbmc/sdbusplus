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
    def load(name, rootdir="."):
        filename = os.path.join(rootdir,
                                name.replace(".", "/") + ".interface.yaml")

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

    def enum_includes(self, inc_list):
        includes = []
        for e in inc_list:
            es = e.enum_namespace(self.name).split("::")
            # Skip empty, non-enum values and self references like '::'
            if len(es) < 2:
                continue
            # All elements will be formatted (x::)+
            # If the requested enum is xyz.openbmc_project.Network.IP.Protocol
            # for a server_* configuration, the enum_namespace will be
            # xyz::openbmc_project::Network::server::IP:: and we need to
            # convert to xyz/openbmc_project/Network/IP/server.hpp
            es.pop()  # Remove trailing empty element
            e_class = es.pop()  # Remove class name
            e_type = es.pop()  # Remove injected type namespace
            es.append(e_class)
            es.append(e_type)
            includes.append("/".join(es) + ".hpp")
        return includes

    def markdown(self, loader):
        return self.render(loader, "interface.md.mako", interface=self)

    def server_header(self, loader):
        return self.render(loader, "interface.server.hpp.mako", interface=self)

    def server_cpp(self, loader):
        return self.render(loader, "interface.server.cpp.mako", interface=self)

    def client_header(self, loader):
        return self.render(loader, "interface.client.hpp.mako", interface=self)
