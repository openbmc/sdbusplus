import re

import inflection


class NamedElement(object):
    def __init__(self, **kwargs):
        super(NamedElement, self).__init__()
        self.name = kwargs.pop("name", "unnamed")
        self.description = kwargs.pop("description", "")

        if not isinstance(self.name, str):
            raise AttributeError(
                "Element interpreted by YAML parser as non-string; likely "
                "missing quotes around original name."
            )

        self.old_namespaces = self.name.split(".")
        self.old_classname = self.old_namespaces.pop()
        self.namespaces = [
            inflection.underscore(x) for x in self.old_namespaces
        ]
        self.classname = inflection.camelize(self.old_classname)

    def __getattribute__(self, name):
        lam = {
            "CamelCase": lambda: inflection.camelize(self.name),
            "camelCase": lambda: NamedElement.lower_camel_case(self.name),
            "snake_case": lambda: inflection.underscore(self.name),
            "SNAKE_CASE": lambda: inflection.underscore(self.name).upper(),
        }.get(name)

        if lam:
            return NamedElement.__fixup_name(lam())
        try:
            return super(NamedElement, self).__getattribute__(name)
        except Exception:
            raise AttributeError(
                "Attribute '%s' not found in %s.NamedElement"
                % (name, self.__module__)
            )

    def old_cppNamespace(self, typename="server"):
        return "::".join(self.old_namespaces) + "::" + typename

    def old_cppNamespacedClass(self, typename="server"):
        return self.old_cppNamespace(typename) + "::" + self.old_classname

    def headerFile(self, typename="common"):
        return self.name.replace(".", "/") + f"/{typename}.hpp"

    def cppNamespace(self):
        return "::".join(self.namespaces)

    def cppNamespacedClass(self):
        return self.cppNamespace() + "::" + self.classname

    def registryPrefix(self, root_prefix):
        name = inflection.camelize(
            self.name.replace("xyz.openbmc_project.", "")
            .replace("com.", "")
            .replace(".", "_"),
            uppercase_first_letter=True,
        )
        return root_prefix + "_" + name

    """ Some names are reserved in some languages.  Fixup names to avoid using
        reserved words.
    """

    @staticmethod
    def __fixup_name(name):
        # List of reserved words from http://en.cppreference.com/w/cpp/keyword
        cppReserved = frozenset(
            {
                "alignas",
                "alignof",
                "and",
                "and_eq",
                "asm",
                "auto",
                "bitand",
                "bitor",
                "bool",
                "break",
                "case",
                "catch",
                "char",
                "char8_t",
                "char16_t",
                "char32_t",
                "class",
                "compl",
                "concept",
                "const",
                "consteval",
                "constexpr",
                "constinit",
                "const_cast",
                "continue",
                "co_await",
                "co_return",
                "co_yield",
                "decltype",
                "default",
                "delete",
                "do",
                "double",
                "dynamic_cast",
                "else",
                "enum",
                "explicit",
                "export",
                "extern",
                "false",
                "float",
                "for",
                "friend",
                "goto",
                "if",
                "inline",
                "int",
                "long",
                "mutable",
                "namespace",
                "new",
                "noexcept",
                "not",
                "not_eq",
                "nullptr",
                "operator",
                "or",
                "or_eq",
                "private",
                "protected",
                "public",
                "register",
                "reinterpret_cast",
                "requires",
                "return",
                "short",
                "signed",
                "sizeof",
                "static",
                "static_assert",
                "static_cast",
                "struct",
                "switch",
                "template",
                "this",
                "thread_local",
                "throw",
                "true",
                "try",
                "typedef",
                "typeid",
                "typename",
                "union",
                "unsigned",
                "using",
                "virtual",
                "void",
                "volatile",
                "wchar_t",
                "while",
                "xor",
                "xor_eq",
            }
        )

        while name in cppReserved:
            name = name + "_"

        return name

    # Normally you can use inflection.camelize(string, False) to return
    # a lowerCamelCase string, but it doesn't work well for acronyms.
    # An input like "MACAddress" will become "mACAddress".  Try to handle
    # acronyms in a better way.
    @staticmethod
    def lower_camel_case(name):
        # Get the UpperCamelCase variation of the name first.
        upper_name = inflection.camelize(name)

        # If it is all upper case, return as all lower.  ex. "MAC"
        if re.match(r"^[A-Z0-9]*$", upper_name):
            return upper_name.lower()

        # If it doesn't start with 2 upper case, it isn't an acronym.
        if not re.match(r"^[A-Z]{2}", upper_name):
            return upper_name[0].lower() + upper_name[1:]

        # If it is upper case followed by 'v[0-9]', treat it all as one word.
        # ex. "IPv6Address" -> "ipv6Address"
        if re.match(r"^[A-Z]+v[0-9]", upper_name):
            return re.sub(
                r"^([A-Z]+)(.*)$",
                lambda m: m.group(1).lower() + m.group(2),
                upper_name,
            )

        # Anything left has at least two sequential upper-case, so it is an
        # acronym.  Switch all but the last upper-case (which starts the next
        # word) to lower-case.
        # ex. "MACAddress" -> "macAddress".
        return re.sub(
            r"^([A-Z]+)([A-Z].*)$",
            lambda m: m.group(1).lower() + m.group(2),
            upper_name,
        )
