import datetime
import itertools
import json
import os

import jsonschema
import yaml

from .namedelement import NamedElement
from .property import Property
from .renderer import Renderer


class EventMetadata(Property):
    def __init__(self, **kwargs):
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
        self.is_error = kwargs.pop("is_error", False)
        self.deprecated = kwargs.pop("deprecated", None)
        self.errno = kwargs.pop("errno", "EIO")
        self.redfish_map = kwargs.pop("redfish-mapping", None)
        if not self.redfish_map:
            self.languages = {
                key: EventLanguage(**kwargs.pop(key, {})) for key in ["en"]
            }
        else:
            self.languages = {
                "en": EventLanguage(
                    **{"message": f"Redfish({self.redfish_map})"}
                )
            }
        self.metadata = [
            EventMetadata(**n) for n in kwargs.pop("metadata", [])
        ]

        self.severity = kwargs.pop("severity", "informational")
        self.syslog_sev = EventElement.syslog_severity(self.severity)
        self.registry_sev = EventElement.registry_severity(self.severity)

        super(EventElement, self).__init__(**kwargs)

    def cpp_includes(self, interface):
        includes = []
        for m in self.metadata:
            includes.extend(m.enum_headers(interface))
        return sorted(set(includes))

    def registry_event(self, interface, language):
        if language not in self.languages:
            language = "en"
        language_data = self.languages[language]

        args = [x for x in self.metadata if x.primary]

        for i, arg in enumerate(args):
            language_data.message = language_data.message.replace(
                f"{{{arg.name}}}", f"%{i+1}"
            )

        result = {}
        if language_data.description:
            result["Description"] = language_data.description
        result["Message"] = language_data.message
        if language_data.resolution:
            result["Resolution"] = language_data.resolution
        else:
            result["Resolution"] = "None."

        result["MessageSeverity"] = self.registry_sev

        result["NumberOfArgs"] = len(args)
        result["ParamTypes"] = [x.registry_type() for x in args]

        result["Oem"] = {
            "OpenBMC_Mapping": {
                "Event": interface + "." + self.name,
                "Args": self.registry_args_mapping(interface),
            }
        }

        return result

    def registry_mapping(self, interface):
        return {
            "RedfishEvent": self.redfish_map,
            "Args": self.registry_args_mapping(interface),
        }

    def registry_args_mapping(self, interface):
        args = [x for x in self.metadata if x.primary]
        return [
            {
                "Name": x.SNAKE_CASE,
                "Type": x.cppTypeParam(interface, full=True),
            }
            for x in args
        ]

    def __getattribute__(self, name):
        lam = {"description": lambda: self.__description()}.get(name)

        if lam:
            return lam()
        try:
            return super(EventElement, self).__getattribute__(name)
        except Exception:
            raise AttributeError(
                "Attribute '%s' not found in %s.EventElement"
                % (name, self.__module__)
            )

    def __description(self):
        en = self.languages["en"]
        if en.description:
            return en.description
        return en.message

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

    @staticmethod
    def registry_severity(severity: str) -> str:
        return {
            "emergency": "Critical",
            "alert": "Critical",
            "critical": "Critical",
            "error": "Warning",
            "warning": "Warning",
            "notice": "Warning",
            "informational": "OK",
            "debug": "OK",
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
        self.errors = [
            EventElement(**n, is_error=True) for n in kwargs.pop("errors", [])
        ]
        self.events = [EventElement(**n) for n in kwargs.pop("events", [])]

        super(Event, self).__init__(**kwargs)

    def cpp_includes(self):
        includes = []
        for e in self.errors:
            includes.extend(e.cpp_includes(self.name))
        for e in self.events:
            includes.extend(e.cpp_includes(self.name))
        return sorted(set(includes))

    def markdown(self, loader):
        return self.render(loader, "events.md.mako", events=self)

    def exception_header(self, loader):
        return self.render(loader, "events.hpp.mako", events=self)

    def exception_cpp(self, loader):
        return self.render(loader, "events.cpp.mako", events=self)

    def exception_registry(self, loader):
        language = "en"
        registryName = self.registryPrefix("OpenBMC")

        messages = {}
        mappings = {}

        for e in itertools.chain(self.errors, self.events):
            if e.redfish_map:
                mappings[self.name + "." + e.name] = e.registry_mapping(
                    self.name
                )
            else:
                messages[e.name] = e.registry_event(self.name, language)

        result = {
            "@Redfish.Copyright": f"Copyright 2024-{datetime.date.today().year} OpenBMC.",
            "@odata.type": "#MessageRegistry.v1_6_3.MessageRegistry",
            "Id": f"{registryName}.{self.version}",
            "Language": f"{language}",
            "Messages": messages,
            "Name": f"OpenBMC Message Registry for {self.name}",
            "Description": f"OpenBMC Message Registry for {self.name}",
            "OwningEntity": "OpenBMC",
            "RegistryPrefix": f"{registryName}",
            "RegistryVersion": f"{self.version}",
        }

        if len(mappings) != 0:
            result["Oem"] = {"OpenBMC_Mapping": mappings}

        return json.dumps(result, indent=4)
