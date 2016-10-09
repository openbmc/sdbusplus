from .property import Property
from .namedelement import NamedElement

class Signal(NamedElement, object):
    def __init__(self, **kwargs):
        self.properties = [ Property(**p) for p in
            kwargs.pop('properties', []) ]

        super(Signal, self).__init__(**kwargs)
