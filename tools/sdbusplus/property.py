from .namedelement import NamedElement
from .renderer import Renderer
import yaml


class Property(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.flags = kwargs.pop('flags', [])
        self.cppFlags = self.cpp_flags(self.flags)
        self.typeName = kwargs.pop('type', None)
        self.cppTypeName = self.parse_cpp_type(self.typeName)
        self.defaultValue = kwargs.pop('default', None)

        super(Property, self).__init__(**kwargs)

    def cpp_flags(self, flags):
        flags_map = {
            "deprecated":"vtable::common_::deprecated",
            "hidden":"vtable::common_::hidden",
            "unprivileged":"vtable::common_::unprivileged",
            "const":"vtable::property_::const_",
            "emits_change":"vtable::property_::emits_change",
            "emits_invalidation":"vtable::property_::emits_invalidation",
            "explicit":"vtable::property_::explicit_"
        }

        l = []
        for flag in flags:
            if flag == 'readonly':
                continue

            r = flags_map.get(flag)
            if r is None:
                raise RuntimeError("Invalid property flag %s" % flag)

            l.append(r)

        return ' | '.join(l)

    def is_enum(self):
        if not self.cppTypeName:
            return False
        if self.cppTypeName.startswith("enum<"):
            return True
        return False

    """ Return a conversion of the cppTypeName valid as a function parameter.
        Currently only 'enum' requires conversion.
    """
    def cppTypeParam(self, interface, server=True):
        if self.is_enum():
            # strip off 'enum<' and '>'
            r = self.cppTypeName.split('>')[0].split('<')[1]

            # self. means local type.
            if r.startswith("self."):
                return r.split('self.')[1]

            r = r.split('.')
            r.insert(-2, "server" if server else "client")
            r = "::".join(r)
            return r
        return self.cppTypeName

    """ Return a conversion of the cppTypeName valid as it is read out of a
        message.  Currently only 'enum' requires conversion.
    """
    def cppTypeMessage(self, interface):
        if self.is_enum():
            return "std::string"
        return self.cppTypeName

    def enum_namespace(self, interface):
        if not self.is_enum():
            return ""
        l = self.cppTypeParam(interface).split("::")[0:-1]
        if len(l) != 0:
            return "::".join(l) + "::"
        return ""

    def enum_name(self, interface):
        if not self.is_enum():
            return ""
        return self.cppTypeParam(interface).split("::")[-1]

    def parse_cpp_type(self, typeName):
        if not typeName:
            return None

        """ typeNames are _almost_ valid YAML.  Insert a , before each [
            and then wrap it in a [ ] and it becomes valid YAML.  (assuming
            the user gave us a valid typename)
        """
        typeArray = yaml.safe_load("[" + ",[".join(typeName.split("[")) + "]")
        typeTuple = self.__preprocess_yaml_type_array(typeArray).pop(0)
        return self.__parse_cpp_type__(typeTuple)

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
            if i < len(typeArray)-1 and type(typeArray[i+1]) is list:
                result.append(
                    (typeArray[i],
                     self.__preprocess_yaml_type_array(typeArray[i+1])))
            else:
                result.append((typeArray[i], []))

        return result

    """ Take a list of dbus types and perform validity checking, such as:
            [ variant [ dict [ int32, int32 ], double ] ]
        This function then converts the type-list into a C++ type string.
    """
    def __parse_cpp_type__(self, typeTuple):
        propertyMap = {
            'byte': {'cppName': 'uint8_t', 'params': 0},
            'boolean': {'cppName': 'bool', 'params': 0},
            'int16': {'cppName': 'int16_t', 'params': 0},
            'uint16': {'cppName': 'uint16_t', 'params': 0},
            'int32': {'cppName': 'int32_t', 'params': 0},
            'uint32': {'cppName': 'uint32_t', 'params': 0},
            'int64': {'cppName': 'int64_t', 'params': 0},
            'uint64': {'cppName': 'uint64_t', 'params': 0},
            'double': {'cppName': 'double', 'params': 0},
            'string': {'cppName': 'std::string', 'params': 0},
            'path': {'cppName': 'sdbusplus::message::object_path',
                     'params': 0},
            'signature': {'cppName': 'sdbusplus::message::signature',
                          'params': 0},
            'array': {'cppName': 'std::vector', 'params': 1},
            'struct': {'cppName': 'std::tuple', 'params': -1},
            'variant': {'cppName': 'sdbusplus::message::variant',
                        'params': -1},
            'dict': {'cppName': 'std::map', 'params': 2},
            'enum': {'cppName': 'enum', 'params': 1, 'noparse': True}}

        if len(typeTuple) != 2:
            raise RuntimeError("Invalid typeTuple %s" % typeTuple)

        first = typeTuple[0]
        entry = propertyMap[first]

        result = entry['cppName']

        # Handle 0-entry parameter lists.
        if (entry['params'] == 0):
            if (len(typeTuple[1]) != 0):
                raise RuntimeError("Invalid typeTuple %s" % typeTuple)
            else:
                return result

        # Get the parameter list
        rest = typeTuple[1]

        # Confirm parameter count matches.
        if (entry['params'] != -1) and (entry['params'] != len(rest)):
            raise RuntimeError("Invalid entry count for %s : %s" %
                               (first, rest))

        # Parse each parameter entry, if appropriate, and create C++ template
        # syntax.
        result += '<'
        if entry.get('noparse'):
            # Do not parse the parameter list, just use the first element
            # of each tuple and ignore possible parameters
            result += ", ".join([e[0] for e in rest])
        else:
            result += ", ".join(map(lambda e: self.__parse_cpp_type__(e),
                                    rest))
        result += '>'

        return result

    def markdown(self, loader):
        return self.render(loader, "property.mako.md", property=self,
                           post=str.strip)
