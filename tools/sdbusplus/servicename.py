import re
import string

from .namedelement import NamedElement

""" Class for parsing 'service_name' definition eleemnts from an interface.
"""


class ServiceName(NamedElement):
    def __init__(self, **kwargs):
        if "default" in kwargs:
            kwargs["name"] = "DefaultService"
            self.value = kwargs.pop("default")
        elif "indexed_prefix" in kwargs:
            # indexed prefix service name such as `xyz.openbmc_project.Host`,
            # which can be used to construct `xyz.openbmc_project.Host{N}`
            # The index must be directly appended, no other construction is intended.
            kwargs["name"] = "IndexedPrefix"
            self.value = kwargs.pop("indexed_prefix")
        else:
            self.value = kwargs.pop("value", False)

        # Validate service name format.
        if len(self.value) == 0:
            raise ValueError("Invalid empty service-name")
        for s in self.value.split("."):
            if len(s) == 0:
                raise ValueError(
                    "Service names cannot have consecutive .:"
                    f" {self.value} {s}"
                )
            if re.search("[^a-zA-Z0-9_]", s):
                raise ValueError(
                    f"Service name contains illegal characters: {self.value}"
                )
            if s[0] in string.digits:
                raise ValueError(
                    "Service name segments may not start with a number:"
                    f" {self.value} {s}"
                )

        super(ServiceName, self).__init__(**kwargs)
