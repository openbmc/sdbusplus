from .namedelement import NamedElement

class Property(NamedElement):
    def __init__(self, **kwargs):
        self.typeName = kwargs.pop('type', None)
        self.defaultValue = kwargs.pop('default', None)

        super(Property, self).__init__(**kwargs)
