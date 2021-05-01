import inflection


class NamedElement(object):
    def __init__(self, **kwargs):
        super(NamedElement, self).__init__()
        self.name = kwargs.pop('name', "unnamed")
        self.description = kwargs.pop('description', "")

    def __getattribute__(self, name):
        lam = {'CamelCase': lambda: inflection.camelize(self.name),
               'camelCase': lambda: inflection.camelize(self.name, False),
               'snake_case': lambda: inflection.underscore(self.name)}\
            .get(name)

        if lam:
            return NamedElement.__fixup_name(lam())
        try:
            return super(NamedElement, self).__getattribute__(name)
        except Exception:
            raise AttributeError("Attribute '%s' not found in %s.NamedElement"
                                 % (name, self.__module__))

    """ Some names are reserved in some languages.  Fixup names to avoid using
        reserved words.
    """
    @staticmethod
    def __fixup_name(name):
        # List of reserved words from http://en.cppreference.com/w/cpp/keyword
        cppReserved = frozenset({
            "alignas", "alignof", "and", "and_eq", "asm", "auto",
            "bitand", "bitor", "bool", "break", "case", "catch", "char",
            "char8_t", "char16_t", "char32_t", "class", "compl", "concept",
            "const", "consteval", "constexpr", "constinit", "const_cast",
            "continue", "co_await", "co_return", "co_yield", "decltype",
            "default", "delete", "do", "double", "dynamic_cast", "else",
            "enum", "explicit", "export", "extern", "false", "float", "for",
            "friend", "goto", "if", "inline", "int", "long", "mutable",
            "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
            "operator", "or", "or_eq", "private", "protected", "public",
            "register", "reinterpret_cast", "requires", "return", "short",
            "signed", "sizeof", "static", "static_assert", "static_cast",
            "struct", "switch", "template", "this", "thread_local", "throw",
            "true", "try", "typedef", "typeid", "typename", "union",
            "unsigned", "using", "virtual", "void", "volatile", "wchar_t",
            "while", "xor", "xor_eq"})

        while(name in cppReserved):
            name = name + "_"

        return name
