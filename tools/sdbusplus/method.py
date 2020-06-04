from .property import Property
from .namedelement import NamedElement
from .renderer import Renderer


class Method(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.parameters = \
            [Property(**p) for p in kwargs.pop('parameters', [])]
        self.returns = \
            [Property(**r) for r in kwargs.pop('returns', [])]
        self.flags = kwargs.pop('flags', [])
        self.cpp_flags = self.or_cpp_flags(self.flags)
        self.errors = kwargs.pop('errors', [])

        super(Method, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "method.md.mako", method=self)

    def cpp_prototype(self, loader, interface, ptype):
        return self.render(loader, "method.prototype.hpp.mako", method=self,
                           interface=interface, ptype=ptype, post=str.rstrip)

    def or_cpp_flags(self, flags):
        """Return the corresponding ORed cpp flags."""
        flags_dict = {"deprecated": "vtable::common_::deprecated",
                      "hidden": "vtable::common_::hidden",
                      "unprivileged": "vtable::common_::unprivileged",
                      "no_reply": "vtable::method_::no_reply"}

        cpp_flags = []
        for flag in flags:
            try:
                cpp_flags.append(flags_dict[flag])
            except KeyError:
                raise ValueError("Invalid flag \"{}\"".format(flag))

        return " | ".join(cpp_flags)
