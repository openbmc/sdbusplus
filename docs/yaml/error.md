# Error YAML

D-Bus errors can be defined by creating a YAML file to describe the errors. From
this YAML file, both documentation and binding code may be generated. The
generated bindings are C++ exception types corresponding to the D-Bus error
name. Ex. `org.freedesktop.Example.Error.SomeError` will create a generated
exception type of `sdbusplus::error::org::freedesktop::example::SomeError` which
may be thrown or caught as appropriate. If the error is thrown from an interface
method which has specified it may return that error, then the bindings will
generate a catch clause that returns a D-Bus error like
"org.freedesktop.Example.Error.SomeError" to the method caller.

The error YAML is simply a list of `name` along with optional `description` and
`errno` properties. Example:

```yaml
- name: SomeError
- name: AnotherError
  description: >
    This is another error.
  errno: E2BIG
```
