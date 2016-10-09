import os
import yaml
from .namedelement import NamedElement
from .property import Property
from .method import Method
from .signal import Signal

class Interface(NamedElement, object):
    @staticmethod
    def load(name, rootdir='.'):
        filename = os.path.join(rootdir,
                                name.replace('.', '/') + ".interface.yaml")

        with open(filename) as f:
            data = f.read();
            y = yaml.safe_load(data)
            y['name'] = name
            return Interface(**y)

    def __init__(self, **kwargs):
        self.properties = [ Property(**p) for p in
            kwargs.pop('properties', []) ]
        self.methods = [ Method(**m) for m in
            kwargs.pop('methods', []) ]
        self.signals = [ Signal(**s) for s in
            kwargs.pop('signals', []) ]

        super(Interface, self).__init__(**kwargs)

    def markdown(self, loader):
        template = loader.get_template("interface.mako.md")
        return template.render(interface=self, loader=loader)
