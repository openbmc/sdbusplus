from .namedelement import NamedElement

class Property(NamedElement,object):
    def __init__(self, values):
        super(Property, self).__init__(values)

        self.typeName = values.get('type')
        self.defaultValue = values.get('default')
