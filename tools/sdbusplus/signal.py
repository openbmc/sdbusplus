from .property import Property
from .namedelement import NamedElement

class Signal(NamedElement):
    def __init__(self, values):
        super().__init__(values)

        self.properties = []

        properties = values.get('properties', [])
        for p in properties:
            self.properties.append(Property(p))

    def markdown(self, loader):
        template = loader.get_template("signal.mako.md")
        return template.render(signal=self, loader=loader)

