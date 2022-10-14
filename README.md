# sdbusplus

sdbusplus contains two parts:

1. A C++ library (libsdbusplus) for interacting with D-Bus, built on top of
   the sd-bus library from systemd.
2. A tool (sdbus++) to generate C++ bindings to simplify the development of
   D-Bus-based applications.

## Dependencies

The sdbusplus library requires sd-bus, which is contained in libsystemd.

The sdbus++ application requires Python 3 and the Python libraries mako
and inflection.

## Building

The sdbusplus library is built using meson.

```sh
meson build
cd build
ninja
ninja test
ninja install
```

Optionally, building the tests and examples can be disabled by passing
`-Dtests=disabled` and `-Dexamples=disabled` respectively to `meson`.

The sdbus++ application is installed as a standard Python package
using `setuptools`.

```sh
cd tools
./setup.py install
```

## C++ library

The sdbusplus library builds on top of the [sd-bus] library to create a modern
C++ API for D-Bus. The library attempts to be as lightweight as possible,
usually compiling to exactly the sd-bus API calls that would have been
necessary, while also providing compile-time type-safety and memory leak
protection afforded by modern C++ practices.

Consider the following code:

```cpp
auto b = bus::new_default_system();
auto m = b.new_method_call("org.freedesktop.login1",
                           "/org/freedesktop/login1",
                           "org.freedesktop.login1.Manager",
                           "ListUsers");
auto reply = b.call(m);

std::vector<std::tuple<uint32_t, std::string, message::object_path>> users;
reply.read(users);
    // or
auto users = reply.unpack<
    std::vector<std::tuple<uint32_t, std::string, message::object_path>>>();
```

In a few, relatively succinct, C++ lines this snippet will create a D-Bus
connection to the system bus, and call the systemd login manager to get a
list of active users. The message and bus objects are automatically freed
when they leave scope and the message format strings are generated at compile
time based on the types being read. Compare this to the corresponding server
code within [logind].

In general, the library attempts to mimic the naming conventions of the sd-bus
library: ex. `sd_bus_call` becomes `sdbusplus::bus::call`,
`sd_bus_get_unique_name` becomes `sdbusplus::bus::get_unique_name`,
`sd_bus_message_get_signature` becomes `sdbusplus::message::get_signature`,
etc. This allows a relatively straight-forward translation back to the sd-bus
functions for looking up the manpage details.

[sd-bus]: http://0pointer.net/blog/the-new-sd-bus-api-of-systemd.html
[logind]: https://github.com/systemd/systemd/blob/d60c527009133a1ed3d69c14b8c837c790e78d10/src/login/logind-dbus.c#L496

## Binding generation tool

sdbusplus also contains a bindings generator tool: `sdbus++`. The purpose of
a bindings generator is to reduce the boilerplate associated with creating
D-Bus server or client applications. When creating a server application,
rather than creating sd-bus vtables and writing C-style functions to handle
each vtable callback, you can create a small YAML file to define your D-Bus
interface and the `sdbus++` tool will create a C++ class that implements your
D-Bus interface. This class has a set of virtual functions for each method
and property, which you can overload to create your own customized behavior
for the interface.

There are currently two types of YAML files: [interface](docs/yaml/interface.md)
and [error](docs/yaml/error.md). Interfaces are used to create server and client
D-Bus interfaces. Errors are used to define C++ exceptions which can be thrown
and will automatically turn into D-Bus error responses.

[[D-Bus client bindings are not yet implemented.  See openbmc/openbmc#851.]]

### Generating bindings

## How to use tools/sdbus++

The path of your file will be the interface name. For example, for an interface
`org.freedesktop.Example`, you would create the files
`org/freedesktop/Example.interface.yaml` and
`org/freedesktop/Example.errors.yaml]` for interfaces and errors respectively.
These can then be used to generate the server and error bindings:

```sh
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

```sh
sdbus++ interface markdown org.freedesktop.Example > \
    org/freedesktop/Example.md
sdbus++ error markdown org.freedesktop.Example >> \
    org/freedesktop/Example.md
```

See the `example/meson.build` for more details.

## Installing sdbusplus on custom distributions

Installation of sdbusplus bindings on a custom distribution requires a few
packages to be installed prior. Although these packages are the same for several
distributions the names of these packages do differ. Below are the packages
needed for Ubuntu and Fedora.

### Installation on Ubuntu

```sh
sudo apt install git meson libtool pkg-config g++ libsystemd-dev \
    python3 python3-pip python3-yaml python3-mako python3-inflection
```

### Installation on Fedora

```sh
sudo dnf install git meson libtool gcc-c++ pkgconfig systemd-devel \
    python3 python3-pip python3-yaml python3-mako
```

Install the inflection package using the pip utility (on Fedora)

```sh
pip3 install inflection
```
