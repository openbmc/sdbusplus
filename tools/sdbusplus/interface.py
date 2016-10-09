import yaml
from .namedelement import NamedElement
from .property import Property
from .method import Method
from .signal import Signal

class Interface(NamedElement):
    def __init__(self, name, *args, **kwargs):
        self.properties = []
        self.methods = []
        self.signals = []

        rootdir = kwargs.get('rootdir', ".")
        filename = rootdir + "/" + name.replace('.', '/') + ".interface.yaml"
        self.__load(name, filename)
        return

    def __load(self, name, filename):
        data = open(filename).read()
        y = yaml.safe_load(data)

        super().__init__(y)
        # Need to reset name because we get it as a parameter and not from the
        # generic 'NamedElement' class.
        self.name = name

        properties = y.get('properties', [])
        for p in properties:
            self.properties.append(Property(p))

        methods = y.get('methods', [])
        for m in methods:
            self.methods.append(Method(m))

        signals = y.get('signals', [])
        for s in signals:
            self.signals.append(Signal(s))
