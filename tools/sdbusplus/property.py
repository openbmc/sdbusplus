from .namedelement import NamedElement
from .renderer import Renderer

class Property(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.typeName = kwargs.pop('type', None)
        self.defaultValue = kwargs.pop('default', None)

        super(Property, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "property.mako.md", property=self).strip()
