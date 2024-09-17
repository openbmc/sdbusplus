import os

import jsonschema
import yaml

from .namedelement import NamedElement
from .renderer import Renderer


class EventMetadata(NamedElement):
    def __init__(self, **kwargs):
        self.type = kwargs.pop("type")
        self.primary = kwargs.pop("primary", False)
        super(EventMetadata, self).__init__(**kwargs)


class EventLanguage(object):
    def __init__(self, **kwargs):
        super(EventLanguage, self).__init__()
        self.description = kwargs.pop("description", False)
        self.message = kwargs.pop("message")
        self.resolution = kwargs.pop("resolution", False)


class EventElement(NamedElement):
    def __init__(self, **kwargs):
        self.deprecated = kwargs.pop("deprecated", None)
        self.errno = kwargs.pop("errno", None)
        self.languages = {
            key: EventLanguage(**kwargs.pop(key, {})) for key in ["en"]
        }
        self.metadata = [
            EventMetadata(**n) for n in kwargs.pop("metadata", [])
        ]
        self.redfish_map = kwargs.pop("redfish-mapping", None)
        self.severity = EventElement.syslog_severity(
            kwargs.pop("severity", "informational")
        )

        super(EventElement, self).__init__(**kwargs)

    @staticmethod
    def syslog_severity(severity: str) -> str:
        return {
            "emergency": "LOG_EMERG",
            "alert": "LOG_ALERT",
            "critical": "LOG_CRIT",
            "error": "LOG_ERR",
            "warning": "LOG_WARNING",
            "notice": "LOG_NOTICE",
            "informational": "LOG_INFO",
            "debug": "LOG_DEBUG",
        }[severity]


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
        self.version = kwargs.pop("version")
        self.errors = [EventElement(**n) for n in kwargs.pop("errors", [])]
        self.events = [EventElement(**n) for n in kwargs.pop("events", [])]

        super(Event, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "events.md.mako", events=self)

    def exception_header(self, loader):
        return self.render(loader, "events.hpp.mako", events=self)

    def exception_cpp(self, loader):
        return self.render(loader, "events.cpp.mako", events=self)
