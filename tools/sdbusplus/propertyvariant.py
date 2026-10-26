from itertools import product
from pathlib import Path

import yaml

from .property import Property
from .renderer import Renderer

TARGET_ALIASES = {
    "size": ("uint32", "uint64"),
    "ssize": ("int32", "int64"),
}


def type_aliases(type_tuple):
    name, params = type_tuple
    aliases = {name} if name in TARGET_ALIASES else set()
    for param in params:
        aliases.update(type_aliases(param))
    return aliases


def resolve_aliases(type_tuple, aliases):
    name, params = type_tuple
    return (
        aliases.get(name, name),
        tuple(resolve_aliases(param, aliases) for param in params),
    )


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


def cpp_types(types):
    return sorted(Property.wire_type_cpp(t) for t in merge_types(types))


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
        return PropertyVariant(types)

    def __init__(self, types):
        self.aliases = sorted({a for t in types for a in type_aliases(t)})
        self.alias_cpp_types = [
            Property.propertyMap[a]["cppName"] for a in self.aliases
        ]
        self.specializations = []
        self.types = cpp_types(types) if not self.aliases else None
        if not self.aliases:
            return

        choices = (TARGET_ALIASES[a] for a in self.aliases)
        for replacements in product(*choices):
            aliases = dict(zip(self.aliases, replacements))
            resolved = [resolve_aliases(t, aliases) for t in types]
            arguments = [
                Property.propertyMap[t]["cppName"] for t in replacements
            ]
            self.specializations.append((arguments, cpp_types(resolved)))

    def common_header(self, loader):
        return self.render(loader, "propertyvariant.hpp.mako", variant=self)
