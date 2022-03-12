import yaml

from .namedelement import NamedElement
from .renderer import Renderer


class Property(NamedElement, Renderer):

    LOCAL_ENUM_MAGIC = "<LOCAL_ENUM>"
    NONLOCAL_ENUM_MAGIC = "<NONLOCAL_ENUM>"

    def __init__(self, **kwargs):
        self.typeName = kwargs.pop("type", None)
        self.cppTypeName = self.parse_cpp_type()
        self.defaultValue = kwargs.pop("default", None)
        self.flags = kwargs.pop("flags", [])
        self.cpp_flags = self.or_cpp_flags(self.flags)
        self.errors = kwargs.pop("errors", [])

        if self.defaultValue is not None:
            if isinstance(self.defaultValue, bool):
                # Convert True/False to 'true'/'false'
                # because it will be rendered as C++ code
                self.defaultValue = "true" if self.defaultValue else "false"
            elif (
                isinstance(self.defaultValue, str)
                and self.typeName.lower() == "string"
            ):
                # Wrap string type default values with double-quotes
                self.defaultValue = '"' + self.defaultValue + '"'
            elif (
                isinstance(self.defaultValue, str) and self.is_floating_point()
            ):
                if self.defaultValue.lower() == "nan":
                    self.defaultValue = (
                        f"std::numeric_limits<{self.cppTypeName}>::quiet_NaN()"
                    )
                elif self.defaultValue.lower() == "infinity":
                    self.defaultValue = (
                        f"std::numeric_limits<{self.cppTypeName}>::infinity()"
                    )
                elif self.defaultValue.lower() == "-infinity":
                    self.defaultValue = (
                        f"-std::numeric_limits<{self.cppTypeName}>::infinity()"
                    )
                elif self.defaultValue.lower() == "epsilon":
                    self.defaultValue = (
                        f"std::numeric_limits<{self.cppTypeName}>::epsilon()"
                    )
            elif isinstance(self.defaultValue, str) and self.is_integer():
                if self.defaultValue.lower() == "maxint":
                    self.defaultValue = (
                        f"std::numeric_limits<{self.cppTypeName}>::max()"
                    )
                elif self.defaultValue.lower() == "minint":
                    self.defaultValue = (
                        f"std::numeric_limits<{self.cppTypeName}>::min()"
                    )

        super(Property, self).__init__(**kwargs)

    def is_enum(self):
        if not self.typeName:
            return False
        return "enum" == self.__type_tuple()[0]

    def is_integer(self):
        return self.typeName in [
            "byte",
            "int16",
            "uint16",
            "int32",
            "uint32",
            "int64",
            "uint64",
            "size",
            "ssize",
        ]

    def is_floating_point(self):
        return self.typeName in ["double"]

    """ Return a conversion of the cppTypeName valid as a function parameter.
        Currently only 'enum' requires conversion.
    """

    def cppTypeParam(self, interface, full=False, server=True):
        return self.__cppTypeParam(interface, self.cppTypeName, full, server)

    def default_value(self):
        if self.defaultValue is not None:
            return " = " + str(self.defaultValue)
        else:
            return ""

    def __cppTypeParam(self, interface, cppTypeName, full=False, server=True):

        ns_type = "server" if server else "client"

        iface = interface.split(".")
        iface.insert(-1, ns_type)
        iface = "::".join(iface)
        iface = "sdbusplus::" + iface

        r = cppTypeName

        # Fix up local enum placeholders.
        if full:
            r = r.replace(self.LOCAL_ENUM_MAGIC, iface)
        else:
            r = r.replace(self.LOCAL_ENUM_MAGIC + "::", "")

        # Fix up non-local enum placeholders.
        r = r.replace(self.NONLOCAL_ENUM_MAGIC, ns_type)

        return r

    """ Determine the C++ namespaces of an enumeration-type property.
    """

    def enum_namespaces(self, interface):
        typeTuple = self.__type_tuple()
        return self.__enum_namespaces(interface, typeTuple)

    def __enum_namespaces(self, interface, typeTuple):
        # Enums can be processed directly.
        if "enum" == typeTuple[0]:
            cppType = self.__cppTypeParam(
                interface, self.__parse_cpp_type__(typeTuple)
            )
            ns = cppType.split("::")[0:-1]
            if len(ns) != 0:
                return ["::".join(ns) + "::"]
            return []

        # If the details part of the tuple has zero entries, no enums are
        # present
        if len(typeTuple[1]) == 0:
            return []

        # Otherwise, the tuple-type might have enums embedded in it.  Handle
        # them recursively.
        r = []
        for t in typeTuple[1]:
            r.extend(self.__enum_namespaces(interface, t))
        return r

    """ Convert the property type into a C++ type.
    """

    def parse_cpp_type(self):
        if not self.typeName:
            return None

        typeTuple = self.__type_tuple()
        return self.__parse_cpp_type__(typeTuple)

    """ Convert the 'typeName' into a tuple of ('type', [ details ]) where
        'details' is a recursive type-tuple.
    """

    def __type_tuple(self):
        if not self.typeName:
            return None

        """ typeNames are _almost_ valid YAML.  Insert a , before each [
            and then wrap it in a [ ] and it becomes valid YAML.  (assuming
            the user gave us a valid typename)
        """
        typeName = self.typeName
        typeArray = yaml.safe_load("[" + ",[".join(typeName.split("[")) + "]")
        return self.__preprocess_yaml_type_array(typeArray).pop(0)

    """ Take a list of dbus types from YAML and convert it to a recursive data
        structure. Each entry of the original list gets converted into a tuple
        consisting of the type name and a list with the params for this type,
        e.g.
            ['dict', ['string', 'dict', ['string', 'int64']]]
        is converted to
            [('dict', [('string', []), ('dict', [('string', []),
             ('int64', [])]]]
    """

    def __preprocess_yaml_type_array(self, typeArray):
        result = []

        for i in range(len(typeArray)):
            # Ignore lists because we merge them with the previous element
            if type(typeArray[i]) is list:
                continue

            # If there is a next element and it is a list, merge it with the
            # current element.
            if i < len(typeArray) - 1 and type(typeArray[i + 1]) is list:
                result.append(
                    (
                        typeArray[i],
                        self.__preprocess_yaml_type_array(typeArray[i + 1]),
                    )
                )
            else:
                result.append((typeArray[i], []))

        return result

    """ Take a list of dbus types and perform validity checking, such as:
            [ variant [ dict [ int32, int32 ], double ] ]
        This function then converts the type-list into a C++ type string.
    """

    def __parse_cpp_type__(self, typeTuple):
        propertyMap = {
            "byte": {"cppName": "uint8_t", "params": 0},
            "boolean": {"cppName": "bool", "params": 0},
            "int16": {"cppName": "int16_t", "params": 0},
            "uint16": {"cppName": "uint16_t", "params": 0},
            "int32": {"cppName": "int32_t", "params": 0},
            "uint32": {"cppName": "uint32_t", "params": 0},
            "int64": {"cppName": "int64_t", "params": 0},
            "uint64": {"cppName": "uint64_t", "params": 0},
            "size": {"cppName": "size_t", "params": 0},
            "ssize": {"cppName": "ssize_t", "params": 0},
            "double": {"cppName": "double", "params": 0},
            "unixfd": {"cppName": "sdbusplus::message::unix_fd", "params": 0},
            "string": {"cppName": "std::string", "params": 0},
            "path": {
                "cppName": "sdbusplus::message::object_path",
                "params": 0,
            },
            "object_path": {
                "cppName": "sdbusplus::message::object_path",
                "params": 0,
            },
            "signature": {
                "cppName": "sdbusplus::message::signature",
                "params": 0,
            },
            "array": {"cppName": "std::vector", "params": 1},
            "set": {"cppName": "std::set", "params": 1},
            "struct": {"cppName": "std::tuple", "params": -1},
            "variant": {"cppName": "std::variant", "params": -1},
            "dict": {"cppName": "std::map", "params": 2},
            "enum": {"cppName": "enum", "params": 1},
        }

        if len(typeTuple) != 2:
            raise RuntimeError("Invalid typeTuple %s" % typeTuple)

        first = typeTuple[0]
        entry = propertyMap[first]

        result = entry["cppName"]

        # Handle 0-entry parameter lists.
        if entry["params"] == 0:
            if len(typeTuple[1]) != 0:
                raise RuntimeError("Invalid typeTuple %s" % typeTuple)
            else:
                return result

        # Get the parameter list
        rest = typeTuple[1]

        # Confirm parameter count matches.
        if (entry["params"] != -1) and (entry["params"] != len(rest)):
            raise RuntimeError(
                "Invalid entry count for %s : %s" % (first, rest)
            )

        # Switch enum for proper type.
        if result == "enum":
            result = rest[0][0]

            # self. means local type.
            if result.startswith("self."):
                return result.replace("self.", self.LOCAL_ENUM_MAGIC + "::")

            # Insert place-holder for header-type namespace (ex. "server")
            result = result.split(".")
            result.insert(-2, self.NONLOCAL_ENUM_MAGIC)
            result = "::".join(result)
            return result

        # Parse each parameter entry, if appropriate, and create C++ template
        # syntax.
        result += "<"
        result += ", ".join([self.__parse_cpp_type__(e) for e in rest])
        result += ">"

        return result

    def markdown(self, loader):
        return self.render(
            loader, "property.md.mako", property=self, post=str.strip
        )

    def cpp_prototype(self, loader, interface, ptype):
        return self.render(
            loader,
            "property.prototype.hpp.mako",
            property=self,
            interface=interface,
            ptype=ptype,
            post=str.rstrip,
        )

    def or_cpp_flags(self, flags):
        """Return the corresponding ORed cpp flags."""
        flags_dict = {
            "const": "vtable::property_::const_",
            "deprecated": "vtable::common_::deprecated",
            "emits_change": "vtable::property_::emits_change",
            "emits_invalidation": "vtable::property_::emits_invalidation",
            "explicit": "vtable::property_::explicit_",
            "hidden": "vtable::common_::hidden",
            "readonly": False,
            "unprivileged": "vtable::common_::unprivileged",
        }

        cpp_flags = []
        for flag in flags:
            try:
                if flags_dict[flag]:
                    cpp_flags.append(flags_dict[flag])
            except KeyError:
                raise ValueError('Invalid flag "{}"'.format(flag))

        return " | ".join(cpp_flags)
