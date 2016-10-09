class NamedElement:
    def __init__(self, values):
        self.name = values.get('name', "unnamed")
        self.description = values.get('description', "")
