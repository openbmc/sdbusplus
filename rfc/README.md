As requested, these are the dbus bindings that intel has been using for a
few components, and are being posted to foster discussion.  Important
features/architectural points:

1. These bindings are driven entirely by code.  No external definitions
are used.  User provided types are deduced and translated into dbus
types using template parameters.
2. Unit tests are able to be run against a single client that tests the
full stack by reusing the dbus connection object.
3. Contains a C++ sytle message class that wraps the raw c dbus message class, and
manages reference counting using object lifetime.  An advantageous result of this is that dbus messages are now exception safe and meet the requirements for RAII to function correctly.
4. Any type that meets the C++ Container concept can be used with the bindings.  This allows the use of both standard containers (set/unordered_map/array) but also third party standard containers (flat_map, flat_set ect).  This also allows flexability in types between dbus producers and consumers.  This allows each producer and consumer to chose the data structure that best meets their needs, without having to loop a second time to fill their custom type.
5. Types can be devolved from their underlying pointers.  For example, in certain scenarios, std::vector<std::string*> might be more efficient to use and pack than std::vector<std::string>.
6. DbusPropertiesServer class allows the construction of a properties and objectmanager compliant interface through an object,interface, property, and method registration scheme.