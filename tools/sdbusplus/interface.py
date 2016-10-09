import yaml
from .namedelement import NamedElement
from .property import Property

class Interface(NamedElement):
    def __init__(self, name, *args, **kwargs):
        self.properties = []

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
