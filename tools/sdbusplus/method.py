from .property import Property
from .namedelement import NamedElement

class Method(NamedElement):
    def __init__(self, values):
        super().__init__(values)
        self.parameters = []
        self.returns = []

        parameters = values.get('parameters', [])
        for p in parameters:
            self.parameters.append(Property(p))

        returns = values.get('returns', [])
        for r in returns:
            self.returns.append(Property(r))

    def markdown(self, loader):
        template = loader.get_template("method.mako.md")
        return template.render(method=self, loader=loader)
