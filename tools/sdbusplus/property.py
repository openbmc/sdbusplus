from .namedelement import NamedElement
from .renderer import Renderer
import yaml


class Property(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.typeName = kwargs.pop('type', None)
        self.cppTypeName = self.parse_cpp_type(self.typeName)
        self.defaultValue = kwargs.pop('default', None)

        super(Property, self).__init__(**kwargs)

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

    def parse_cpp_type(self, typeName):
        if not typeName:
            return None

        """ typeNames are _almost_ valid YAML.  Insert a , before each [
            and then wrap it in a [ ] and it becomes valid YAML.  (assuming
            the user gave us a valid typename)
        """
        parse = yaml.safe_load("[" + ",[".join(typeName.split("[")) + "]")
        return self.__parse_cpp_type__(parse)

    """ Take a list of dbus types and perform validity checking, such as:
            [ variant [ dict [ int32, int32 ], double ] ]
        This function then converts the type-list into a C++ type string.
    """
    def __parse_cpp_type__(self, typeArray):
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
            'path': {'cppName': 'std::string', 'params': 0},
            'signature': {'cppName': 'std::string', 'params': 0},
            'array': {'cppName': 'std::vector', 'params': 1},
            'struct': {'cppName': 'std::tuple', 'params': -1},
            'variant': {'cppName': 'sdbusplus::variant', 'params': -1},
            'dict': {'cppName': 'std::map', 'params': 2},
            'enum': {'cppName': 'enum', 'params': 1, 'noparse': True}}

        first = typeArray.pop(0)
        entry = propertyMap[first]

        result = entry['cppName']

        # Handle 0-entry parameter lists.
        if (entry['params'] == 0):
            if (len(typeArray) != 0):
                raise RuntimeError("Invalid typeArray %s" % typeArray)
            else:
                return result

        # Get remainder of the parameter list, which should be the last
        # element.
        rest = typeArray.pop(0)
        if (len(typeArray) != 0):
            raise RuntimeError("Invalid typeArray %s" % typeArray)

        # Confirm parameter count matches.
        if (entry['params'] != -1) and (entry['params'] != len(rest)):
            raise RuntimeError("Invalid entry count for %s : %s" %
                               (first, rest))

        # Parse each parameter entry, if appropriate, and create C++ template
        # syntax.
        result += '<'
        if entry.get('noparse'):
            result += ", ".join(rest)
        else:
            result += ", ".join(map(lambda e: self.__parse_cpp_type__([e]),
                                    rest))
        result += '>'

        return result

    def markdown(self, loader):
        return self.render(loader, "property.mako.md", property=self,
                           post=str.strip)
