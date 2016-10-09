from .property import Property
from .namedelement import NamedElement

class Method(NamedElement, object):
    def __init__(self, values):
        super(Method, self).__init__(values)
        self.parameters = []
        self.returns = []

        parameters = values.get('parameters', [])
        for p in parameters:
            self.parameters.append(Property(p))

        returns = values.get('returns', [])
        for r in returns:
            self.returns.append(Property(r))
