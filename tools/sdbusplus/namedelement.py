class NamedElement(object):
    def __init__(self, **kwargs):
        self.name = kwargs.pop('name', "unnamed")
        self.description = kwargs.pop('description', "")
        super(NamedElement, self).__init__(**kwargs)
