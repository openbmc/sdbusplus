# sdbusplus

sdbusplus contains two parts:

1. A C++ library (libsdbusplus) for interacting with D-Bus, built on top of
   the sd-bus library from systemd.
2. A tool (sdbus++) to generate C++ bindings to simplify the development of
   D-Bus-based applications.

## Dependencies

The sdbusplus library requires sd-bus, which is contained in libsystemd.

The sdbus++ application requires python and the python libraries mako
and py-inflection.

## C++ library

The sdbusplus library builds on top of the
[sd-bus](http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html)
library to create a modern C++ API for D-Bus.  The library attempts to be
as lightweight as possible, usually compiling to exactly the sd-bus API
calls that would have been necessary, while also providing compile-time
type-safety and memory leak protection afforded by modern C++ practices.

Consider the following code:
```
auto b = bus::new_system();
auto m = b.new_method_call("org.freedesktop.login1",
                           "/org/freedesktop/login1",
                           "org.freedesktop.login1.Manager",
                           "ListUsers");
auto reply = b.call(m);

std::vector<std::tuple<uint32_t, std::string, message::object_path>> users;
reply.read(users);
```

In a few, relatively succinct, C++ lines this snippet will create a D-Bus
connection to the system bus, and call the systemd login manager to get a
list of active users.  The message and bus objects are automatically freed
when they leave scope and the message format strings are generated at compile
time based on the types being read.  Compare this to the corresponding server
code within [logind](https://github.com/systemd/systemd/blob/d60c527009133a1ed3d69c14b8c837c790e78d10/src/login/logind-dbus.c#L496).

In general, the library attempts to mimic the naming conventions of the sd-bus
library: ex. `sd_bus_call` becomes `sdbusplus::bus::call`,
`sd_bus_get_unique_name` becomes `sdbusplus::bus::get_unique_name`,
`sd_bus_message_get_signature` becomes `sdbusplus::message::get_signature`,
etc.  This allows a relatively straight-forward translation back to the sd-bus
functions for looking up the manpage details.

## Binding generation tool

sdbusplus also contains a bindings generator tool: `sdbus++`.  The purpose of
a bindings generator is to reduce the boilerplate associated with creating
D-Bus server or client applications.  When creating a server application,
rather than creating sd-bus vtables and writing C-style functions to handle
each vtable callback, you can create a small YAML file to define your D-Bus
interface and the `sdbus++` tool will create a C++ class that implements your
D-Bus interface.  This class has a set of virtual functions for each method
and property, which you can overload to create your own customized behavior
for the interface.

There are currently two types of YAML files: [interface](docs/interface.md) and
[error](docs/error.md).  Interfaces are used to create server and client D-Bus
interfaces.  Errors are used to define C++ exceptions which can be thrown and
will automatically turn into D-Bus error responses.

[[ D-Bus client bindings are not yet implemented.  See openbmc/openbmc#851. ]]

### Generating bindings

## How to use tools/sdbus++

The path of your file will be the interface name. For example, for an interface
`org.freedesktop.Example`, you would create the files
`org/freedesktop/Example.interface.yaml` and
`org/freedesktop/Example.errors.yaml]` for interfaces and errors respectively.
These can then be used to generate the server and error bindings:
```
sdbus++ interface server-header org.freedesktop.Example > \
    org/freedesktop/Example/server.hpp
sdbus++ interface server-cpp org.freedesktop.Example > \
    org/freedesktop/Example/server.cpp
sdbus++ error exception-header org.freedesktop.Example > \
    org/freedesktop/Example/error.hpp \
sdbus++ error exception-cpp org.freedesktop.Example > \
    org/freedesktop/Example/error.cpp
```

Markdown-based documentation can also be generated from the interface and
exception files:
```
sdbus++ interface markdown org.freedesktop.Example > \
    org/freedesktop/Example.md
sdbus++ error markdown org.freedesktop.Example >> \
    org/freedesktop/Example.md
```

See the `example/Makefile.am` for more details.
