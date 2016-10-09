import os
import yaml
from .namedelement import NamedElement
from .property import Property
from .method import Method

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

        super(Interface, self).__init__(values)

        properties = values.get('properties', [])
        for p in properties:
            self.properties.append(Property(p))

        methods = values.get('methods', [])
        for m in methods:
            self.methods.append(Method(m))
