from .namedelement import NamedElement
from .property import Property

class Enum(NamedElement):
    def __init__(self, **kwargs):
        self.values = \
            [Property(**v) for v in kwargs.pop('values', [])]

        super(Enum, self).__init__(**kwargs)
