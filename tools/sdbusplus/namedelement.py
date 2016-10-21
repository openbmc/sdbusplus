import inflection


class NamedElement(object):
    def __init__(self, **kwargs):
        self.name = kwargs.pop('name', "unnamed")
        self.description = kwargs.pop('description', "")
        super(NamedElement, self).__init__(**kwargs)

    def __getattr__(self, name):
        l = {'CamelCase': lambda: inflection.camelize(self.name),
             'camelCase': lambda: inflection.camelize(self.name, False),
             'snake_case': lambda: inflection.underscore(self.name)}\
            .get(name)

        if l:
            return l()
        try:
            return super(NamedElement, self).__getattr__(name)
        except:
            raise AttributeError("Attribute '%s' not found in %s.NamedElement"
                                 % (name, self.__module__))
