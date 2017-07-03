# Error YAML

dbus errors can be defined by creating a YAML file to describe the errors.
From this YAML file, both documentation and binding code may be generated.
The generated bindings are C++ exception types corresponding to the dbus
error name.  Ex. `org.freedesktop.Example.Error.SomeError` will create a
generated exception type of
`sdbusplus::org::freedesktop::Example::Error::SomeError` which may be thrown
or caught as appropriate.  If the error is thrown from an interface method
which has specified it may return that error, then the bindings will generate
a catch clause the returns a dbus error like
"org.freedesktop.Example.Error.SomeError" to the method caller.

The error YAML is simply a list of `name` and optional `description` properties.
Example:
```
- name: SomeError
- name: AnotherError
  description: >
    This is another error.
```

