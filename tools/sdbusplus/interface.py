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
            return Interface(y)

    def __init__(self, values):
        self.properties = []
        self.methods = []
        self.signals = []

        super(Interface, self).__init__(values)

        properties = values.get('properties', [])
        for p in properties:
            self.properties.append(Property(p))

        methods = values.get('methods', [])
        for m in methods:
            self.methods.append(Method(m))

        signals = values.get('signals', [])
        for s in signals:
            self.signals.append(Signal(s))

    def markdown(self, loader):
        template = loader.get_template("interface.mako.md")
        return template.render(interface=self, loader=loader)
