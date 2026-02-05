import os

import yaml
import sys

from glob import glob
from .structNode import StructNode
from .namedelement import NamedElement
from .interface import Interface
from .renderer import Renderer

def find_yaml(directory):
    result = [y for x in os.walk(directory) for y in glob(os.path.join(x[0], '*.interface.yaml'))]
    return result

class FullTree(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir, schemadir):

        print("load name="+name, file=sys.stderr)

        filenames = sorted(find_yaml(rootdir))

        relPaths = [x[len(rootdir)+1:] for x in filenames]

        interfaceNames = [x[:-len(".interface.yaml")] for x in relPaths]

        interfaces = []

        for intf in interfaceNames:
            filename = os.path.join(
                rootdir, intf + ".interface.yaml"
            )
            print("found filename "+filename, file=sys.stderr)

            with open(filename) as f:
                data = f.read()
                y = yaml.safe_load(data)
                y["name"] = intf.replace("/", ".")
                interfaces.append(y)

        return FullTree(interfaces=interfaces, paths=relPaths)

    def __init__(self, **kwargs):
        self.interfaces = [Interface(**p) for p in kwargs.pop("interfaces", [])]
        self.paths = [p for p in kwargs.pop("paths", [])]

        print("receive "+str(len(self.paths))+" paths", file=sys.stderr)

        self.structNodes = {}

        for i, path in enumerate(self.paths):
            intf = self.interfaces[i]
            segments = path.split("/")

            # get rid of ".interface.yaml"
            segments[-1] = segments[-1].split(".")[0]

            if segments[0] not in self.structNodes:
                self.structNodes[segments[0]] = StructNode(intf=intf, origSegments=segments, depth=1)

            self.structNodes[segments[0]].add(intf, segments)

        super(FullTree, self).__init__(**kwargs)

    def tree_header(self, loader):
        return self.render(loader, "full_tree.hpp.mako", interfaces=self.interfaces, structNodes=self.structNodes)

