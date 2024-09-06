import os

import jsonschema
import yaml

from .namedelement import NamedElement
from .renderer import Renderer


class Event(NamedElement, Renderer):
    @staticmethod
    def load(name, rootdir, schemadir):
        schemafile = os.path.join(schemadir, "events.schema.yaml")
        with open(schemafile) as f:
            data = f.read()
            schema = yaml.safe_load(data)

            spec = jsonschema.Draft202012Validator
            spec.check_schema(schema)

            validator = spec(schema)

        filename = os.path.join(
            rootdir, name.replace(".", "/") + ".events.yaml"
        )

        with open(filename) as f:
            data = f.read()
            y = yaml.safe_load(data)

            validator.validate(y)

            y["name"] = name
            return Event(**y)

    def __init__(self, **kwargs):
        super(Event, self).__init__(**kwargs)

    def markdown(self, loader):
        return ""

    def exception_header(self, loader):
        return ""

    def exception_cpp(self, loader):
        return ""
