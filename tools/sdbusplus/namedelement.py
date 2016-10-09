class NamedElement(object):
    def __init__(self, values):
        self.name = values.get('name', "unnamed")
        self.description = values.get('description', "")
