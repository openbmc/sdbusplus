import re

from .namedelement import NamedElement

""" Class for parsing 'path' definition elements from an interface.
"""


class Path(NamedElement):
    def __init__(self, segment=False, **kwargs):
        if "namespace" in kwargs:
            kwargs["name"] = "NamespacePath"
            self.value = kwargs.pop("namespace")
        elif "instance" in kwargs:
            kwargs["name"] = "InstancePath"
            self.value = kwargs.pop("instance")
        else:
            self.value = kwargs.pop("value", "")

        # Validate path/segment format.
        if len(self.value) == 0:
            if segment:
                self.value = NamedElement(name=kwargs["name"]).snake_case
            else:
                raise ValueError("Invalid empty path.")
        if not segment and self.value[0] != "/":
            raise ValueError(f"Paths must start with /: {self.value}")
        if segment and self.value[0] == "/":
            raise ValueError(f"Segments cannot start with /: {self.value}")
        segments = self.value.split("/")
        if not segment:
            segments = segments[1:]
        for s in segments:
            if len(s) == 0:
                raise ValueError(
                    f"Paths cannot have consecutive /: {self.value} {s}"
                )
            if re.search("[^a-zA-Z0-9_]", s):
                raise ValueError(
                    f"Path contains illegal characters: {self.value}"
                )

        self.segments = [
            Path(segment=True, **s) for s in kwargs.pop("segments", [])
        ]

        super(Path, self).__init__(**kwargs)
