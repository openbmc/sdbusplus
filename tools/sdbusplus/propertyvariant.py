from pathlib import Path

import yaml

from .property import Property
from .renderer import Renderer


def type_shape(type_tuple):
    name, params = type_tuple
    if name == "variant":
        return (name, ())
    return (name, tuple(type_shape(t) for t in params))


def merge_type(left, right):
    name, params = left
    if name == "variant":
        return (name, tuple(merge_types(params + right[1])))
    return (name, tuple(merge_type(a, b) for a, b in zip(params, right[1])))


def merge_types(types):
    merged = {}
    for entry in types:
        # Normalize nested variants even when this is the first such shape.
        name, params = entry
        if name == "variant":
            entry = (name, tuple(merge_types(params)))
        else:
            entry = (name, tuple(merge_types((p,))[0] for p in params))
        shape = type_shape(entry)
        if shape in merged:
            entry = merge_type(merged[shape], entry)
        merged[shape] = entry
    return sorted(merged.values())


class PropertyVariant(Renderer):
    @staticmethod
    def load(name, rootdir, schemadir):
        types = set()
        for filename in sorted(Path(rootdir).rglob("*.interface.yaml")):
            with filename.open() as source:
                interface = yaml.safe_load(source)
            for definition in interface.get("properties", []):
                prop = Property(**definition)
                if not prop.uses_property_variant():
                    types.add(prop.wire_type())
        if not types:
            raise ValueError("No non-recursive property types found")
        return PropertyVariant(
            sorted(Property.wire_type_cpp(t) for t in merge_types(types))
        )

    def __init__(self, types):
        self.types = types

    def common_header(self, loader):
        return self.render(loader, "propertyvariant.hpp.mako", variant=self)
