from .namedelement import NamedElement
from .property import Property
from .renderer import Renderer


class Method(NamedElement, Renderer):
    def __init__(self, **kwargs):
        self.parameters = [Property(**p) for p in kwargs.pop("parameters", [])]
        self.returns = [Property(**r) for r in kwargs.pop("returns", [])]
        self.flags = kwargs.pop("flags", [])
        self.cpp_flags = self.or_cpp_flags(self.flags)
        self.errors = kwargs.pop("errors", [])

        super(Method, self).__init__(**kwargs)

    def markdown(self, loader):
        return self.render(loader, "method.md.mako", method=self)

    def cpp_prototype(self, loader, interface, ptype):
        return self.render(
            loader,
            "method.prototype.hpp.mako",
            method=self,
            interface=interface,
            ptype=ptype,
            post=str.rstrip,
        )

    def cpp_includes(self, interface):
        return interface.error_includes(self.errors) + interface.enum_includes(
            self.returns + self.parameters
        )

    def returns_as_list(self, interface, full=False):
        return ", ".join(
            [r.cppTypeParam(interface.name, full=full) for r in self.returns]
        )

    def cpp_return_type(self, interface):
        if len(self.returns) == 0:
            return "void"
        elif len(self.returns) == 1:
            return self.returns[0].cppTypeParam(interface.name)
        else:
            return "std::tuple<" + self.returns_as_list(interface) + ">"

    def parameter(self, interface, p, defaultValue=False, ref=""):
        r = "%s%s %s" % (p.cppTypeParam(interface.name), ref, p.camelCase)
        if defaultValue:
            r += p.default_value()
        return r

    def parameters_as_list(
        self, transform=lambda p: p.camelCase, join_str=", "
    ):
        return join_str.join([transform(p) for p in self.parameters])

    def parameters_as_arg_list(self, interface):
        return self.parameters_as_list(
            lambda p: self.parameter(interface, p, ref="&&")
        )

    def parameter_types_as_list(self, interface):
        return self.parameters_as_list(
            lambda p: p.cppTypeParam(interface.name, full=True)
        )

    def get_parameters_str(
        self, interface, defaultValue=False, join_str=",\n            "
    ):
        return self.parameters_as_list(
            lambda p: self.parameter(interface, p, defaultValue),
            join_str,
        )

    def or_cpp_flags(self, flags):
        """Return the corresponding ORed cpp flags."""
        flags_dict = {
            "deprecated": "vtable::common_::deprecated",
            "hidden": "vtable::common_::hidden",
            "unprivileged": "vtable::common_::unprivileged",
            "no_reply": "vtable::method_::no_reply",
        }

        cpp_flags = []
        for flag in flags:
            try:
                cpp_flags.append(flags_dict[flag])
            except KeyError:
                raise ValueError('Invalid flag "{}"'.format(flag))

        return " | ".join(cpp_flags)
