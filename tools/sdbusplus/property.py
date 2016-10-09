from .namedelement import NamedElement

class Property(NamedElement,object):
    def __init__(self, values):
        super(Property, self).__init__(values)

        self.typeName = values.get('type')
        self.defaultValue = values.get('default')

    def markdown(self, loader):
        template = loader.get_template("property.mako.md")
        return template.render(property=self, loader=loader).strip()
