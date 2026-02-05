import argparse
import os

import mako.lookup

import sdbusplus


def main():
    module_path = os.path.dirname(sdbusplus.__file__)

    valid_types = {
        "error": sdbusplus.Error,
        "event": sdbusplus.Event,
        "interface": sdbusplus.Interface,
        "full-tree": sdbusplus.FullTree,
    }
    valid_processes = {
        "aserver-header": "async_server_header",
        "client-header": "client_header",
        "common-header": "common_header",
        "tree-header": "tree_header",
        "exception-cpp": "exception_cpp",
        "exception-header": "exception_header",
        "exception-registry": "exception_registry",
        "markdown": "markdown",
        "server-cpp": "server_cpp",
        "server-header": "server_header",
    }

    parser = argparse.ArgumentParser(description="Process sdbus++ YAML files.")

    parser.add_argument(
        "-r",
        "--rootdir",
        dest="rootdir",
        default=".",
        type=str,
        help="Location of files to process.",
    )
    parser.add_argument(
        "-t",
        "--templatedir",
        dest="templatedir",
        default=os.path.join(module_path, "templates"),
        type=str,
        help="Location of templates files.",
    )
    parser.add_argument(
        "-s",
        "--schemadir",
        dest="schemadir",
        default=os.path.join(module_path, "schemas"),
        type=str,
        help="Location of schema files.",
    )
    parser.add_argument(
        "typeName",
        metavar="TYPE",
        type=str,
        choices=valid_types.keys(),
        help="Type to operate on.",
    )
    parser.add_argument(
        "process",
        metavar="PROCESS",
        type=str,
        choices=valid_processes.keys(),
        help="Process to apply.",
    )
    parser.add_argument(
        "item",
        metavar="ITEM",
        type=str,
        help="Item to process.",
    )

    args = parser.parse_args()

    lookup = mako.lookup.TemplateLookup(directories=[args.templatedir])

    instance = valid_types[args.typeName].load(
        args.item, args.rootdir, args.schemadir
    )

    function = getattr(instance, valid_processes[args.process])
    print(function(lookup))
