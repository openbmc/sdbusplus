import tempfile
import unittest
from pathlib import Path

import mako.lookup
import yaml

from sdbusplus.interface import Interface
from sdbusplus.property import Property
from sdbusplus.propertyvariant import PropertyVariant


class PropertyVariantTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.loader = mako.lookup.TemplateLookup(
            directories=[Path(__file__).parent / "sdbusplus/templates"]
        )

    def write_interface(self, name, **definition):
        filename = self.root / (name + ".interface.yaml")
        filename.parent.mkdir(parents=True, exist_ok=True)
        filename.write_text(yaml.safe_dump(definition))

    def test_collect_only_properties_and_deduplicate(self):
        self.write_interface(
            "a/One",
            properties=[{"name": "First", "type": "array[uint64]"}],
            methods=[{"returns": [{"type": "int16"}]}],
            signals=[{"properties": [{"type": "byte"}]}],
        )
        self.write_interface(
            "b/Two",
            properties=[
                {"name": "Second", "type": "array[uint64]"},
                {"name": "Flag", "type": "boolean"},
            ],
        )
        variant = PropertyVariant.load("all", self.root, None)
        self.assertEqual(variant.types, ["bool", "std::vector<uint64_t>"])

    def test_exclude_recursive_properties_not_whole_interfaces(self):
        self.write_interface(
            "Recursive",
            properties=[
                {"name": "Direct", "type": "variant[all]"},
                {"name": "Nested", "type": "dict[string, variant[all]]"},
                {"name": "Ordinary", "type": "string"},
            ],
        )
        variant = PropertyVariant.load("all", self.root, None)
        self.assertEqual(variant.types, ["std::string"])

    def test_wire_types(self):
        cases = {
            "enum[self.State]": "std::string",
            "array[enum[other.Interface.State]]": "std::vector<std::string>",
            "set[string]": "std::vector<std::string>",
            "struct[enum[self.State], object_path]": (
                "std::tuple<std::string, sdbusplus::object_path>"
            ),
            "variant[enum[self.State], string]": (
                "sdbusplus::utility::dedup_variant_t<std::string, std::string>"
            ),
        }
        for name, expected in cases.items():
            with self.subTest(name=name):
                self.assertEqual(Property(type=name).wire_cpp_type(), expected)

    def test_aggregate_type_and_includes_in_all_member_kinds(self):
        prop = {"name": "Value", "type": "dict[string, variant[ all ]]"}
        interface = Interface(
            name="example.Manager",
            properties=[prop],
            methods=[{"name": "Get", "returns": [prop]}],
            signals=[{"name": "Added", "properties": [prop]}],
        )
        self.assertIn(
            "sdbusplus/property_variant.hpp", interface.cpp_includes()
        )
        self.assertEqual(
            interface.properties[0].cppTypeParam(interface.name),
            "std::map<std::string, sdbusplus::PropertyVariant>",
        )

    def test_header_does_not_depend_on_interface_headers(self):
        self.write_interface(
            "Example",
            properties=[{"name": "State", "type": "enum[self.State]"}],
        )
        variant = PropertyVariant.load("all", self.root, None)
        header = variant.common_header(self.loader)
        self.assertIn("using PropertyVariant =", header)
        self.assertNotIn("Example/common.hpp", header)
        self.assertNotIn("State", header)
        self.assertEqual(header, variant.common_header(self.loader))

    def test_merge_variant_properties_with_the_same_wire_shape(self):
        self.write_interface(
            "Variants",
            properties=[
                {"name": "First", "type": "dict[string,variant[int64]]"},
                {"name": "Second", "type": "dict[string,variant[string]]"},
            ],
        )
        variant = PropertyVariant.load("all", self.root, None)
        self.assertEqual(
            variant.types,
            [
                "std::map<std::string, "
                "sdbusplus::utility::dedup_variant_t<int64_t, std::string>>"
            ],
        )

    def test_empty_collection(self):
        with self.assertRaisesRegex(ValueError, "No non-recursive"):
            PropertyVariant.load("all", self.root, None)


if __name__ == "__main__":
    unittest.main()
