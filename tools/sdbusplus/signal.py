from .property import Property
from .namedelement import NamedElement

class Signal(NamedElement, object):
    def __init__(self, values):
        super(Signal, self).__init__(values)

        self.properties = []

        properties = values.get('properties', [])
        for p in properties:
            self.properties.append(Property(p))
