from .property import Property
from .namedelement import NamedElement

class Method(NamedElement):
    def __init__(self, **kwargs):
        self.parameters = [ Property(**p) for p in
            kwargs.pop('parameters', []) ]
        self.returns = [ Property(**r) for r in
            kwargs.pop('returns', []) ]
        self.errors = kwargs.pop('errors', [])

        super(Method, self).__init__(**kwargs)

    def markdown(self, loader):
        template = loader.get_template("method.mako.md")
        return template.render(method=self, loader=loader)
