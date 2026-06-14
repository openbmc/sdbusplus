# ASIO

## Properties

Properties can be dynamically registered to an interface by calling
`register_property()`. If you need to provide a custom getter or setter for a
property, it should follow the guidelines in this section.

To return a D-Bus error reply for either GetProperty or SetProperty, throw a
custom exception derived from `sdbusplus::exception_t`. For the custom
exception, you can return a well-defined [org.freedesktop.DBus.Error][1] from
the `name()` or a custom/arbitrary name. The former will be automatically
translated into a matching [error code][2] that can be consumed by the caller,
while the latter will always be mapped to `EIO`, requiring a `strcmp` to
determine the exact error name.

The handler may also throw any exception not derived from
`sdbusplus::exception_t`, in which case a generic
org.freedesktop.DBus.Error.InvalidArgs error will be returned to the caller.

[1]: https://www.freedesktop.org/software/systemd/man/sd-bus-errors.html#
[2]:
  https://github.com/systemd/systemd/blob/485c9e19e7ebcd912d5fbf11f40afc62951173f8/src/libsystemd/sd-bus/bus-error.c

### Get Property callback

When registering a property using ASIO methods, the get callback should meet the
following prototype:

```c++
PropertyType getHandler(const PropertyType& value);
```

The first argument is the current value cached in the D-Bus interface (i.e. the
previously "getted" value, or the initialized value), and the return value is
returned to the D-Bus caller.

### Set Property callback

When registering a writable property using ASIO methods, the set callback should
meet the following prototype:

```c++
bool setHandler(const PropertyType& newValue, PropertyType& value);
```

The first argument is the new value requested to be set by the D-Bus caller.

The second argument is the actual underlying value contained within the object
server. If the new value meets the expected constraints, the handler must set
`value = newValue` to make the set operation take effect and return true. If the
new value is invalid or cannot be applied for whatever reason, the handler must
leave `value` unmodified and return false.

If the handler returns false but doesn't throw an exception, a generic
org.freedesktop.DBus.Error.InvalidArgs error will be returned to the caller.

## Methods

### Deferred method callback

A method registered with `register_method()` replies as soon as the handler
returns. When the result is not available until a later callback runs (i.e. the
handler hands off to a callback-based asynchronous API), register the method
with `register_completion_method()` instead. The handler then takes an
`sdbusplus::asio::completion` as its last argument:

```c++
iface->register_completion_method(
    "Trigger",
    [](std::string arg, sdbusplus::asio::completion<int32_t> done) {
        startAsyncWork(arg, [done = std::move(done)](
                                boost::system::error_code ec,
                                int32_t result) mutable {
            done(ec, result);
        });
    });
```

The completion's call signature is
`void(boost::system::error_code, Results...)`, the ordinary asio
completion-handler shape, where `Results...` are the method's return types.
Invoke it exactly once: on success pass a default-constructed `error_code`
followed by the return values; on failure pass a non-zero `error_code`, whose
errno is returned to the caller via `new_method_errno()`. Because it has that
signature, the completion can be passed directly to other asio operations as
their completion token. sd-bus enforces the client-side method timeout, so the
handler need not implement one.
