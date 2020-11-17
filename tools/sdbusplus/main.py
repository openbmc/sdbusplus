import sdbusplus
import mako.lookup
import argparse
import sys
import os
import json

valid_types = {"interface": sdbusplus.Interface, "error": sdbusplus.Error}
valid_processes = {
    "markdown": "markdown",
    "server-header": "server_header",
    "server-cpp": "server_cpp",
    "exception-header": "exception_header",
    "exception-cpp": "exception_cpp",
    "client-header": "client_header",
}

def parse_args():
    module_path = os.path.dirname(sdbusplus.__file__)
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

    parser.add_argument('-p',
        '--parameters',
        help='JSON dictionary of parameters to pass to the template')    

    return parser.parse_args()

def get_template_parameters(args):
    parameters = None
    if args.parameters:
        try:
            parameters = json.loads(args.parameters)
        except ValueError as detail:
            sys.stderr.write('Malformed JSON given for parameters: %s\n' % detail)
            sys.exit(2)

        if not isinstance(parameters, dict):
            sys.stderr.write('JSON parameters must be a dictionary\n')
            sys.exit(2)
    return parameters

def main():

    args = parse_args()
    parameters = get_template_parameters(args)
    lookup = mako.lookup.TemplateLookup(directories=[args.templatedir])

    instance = valid_types[args.typeName].load(args.item, args.rootdir)
    function = getattr(instance, valid_processes[args.process])
    print(function(lookup, parameters))
