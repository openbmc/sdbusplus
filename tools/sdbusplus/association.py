from .namedelement import NamedElement
from .renderer import Renderer


class ReverseAssociation(NamedElement, Renderer):

    def __init__(self, **kwargs):

        reverse_name = kwargs.pop("reverse_name", "")
        dict1 = {"name": reverse_name}

        super(ReverseAssociation, self).__init__(**dict1)


class Association(NamedElement, Renderer):

    def __init__(self, **kwargs):

        self.description = kwargs.pop("description", None)
        self.required_endpoint_interfaces = kwargs.pop(
            "required_endpoint_interfaces", None
        )

        reverse_name = kwargs.pop("reverse_name", "")

        self.reverse = ReverseAssociation(reverse_name=reverse_name)

        super(Association, self).__init__(**kwargs)
