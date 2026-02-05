import sys

from .namedelement import NamedElement
from .renderer import Renderer

class StructNode(NamedElement, Renderer):
    def __init__(self, **kwargs):

        self.interface = kwargs.pop("intf")
        self.origSegments = kwargs.pop("origSegments")
        self.depth = kwargs.pop("depth")

        self.members = {}
        self.hasInterface = len(self.origSegments) == self.depth
        self.nodeName = self.origSegments[self.depth-1]

        super(StructNode, self).__init__(**kwargs)

        if (len(self.origSegments) > self.depth):
            self.add(self.interface, self.origSegments)

    def getStructName(self):
        return "_".join(self.origSegments[:self.depth])+"_t"

    def add(self, intf, origSegments):

        if (self.depth >= len(origSegments)):
            return

        seg = origSegments[self.depth]

        if (seg in self.members):
            self.members[seg].add(intf, origSegments)
        else:
            self.members[seg] = StructNode(intf=intf, origSegments=origSegments, depth=self.depth+1)

