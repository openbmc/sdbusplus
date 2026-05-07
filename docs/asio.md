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

### Synchronous methods

`register_method()` registers a synchronous handler. The handler runs to
completion inside the D-Bus dispatch and its return value becomes the method
reply.

### Coroutine methods

If the handler's first parameter is a `boost::asio::yield_context`,
`register_method()` runs the handler in a Boost.Asio stackful coroutine and
sends the reply when the coroutine returns. This path requires Boost coroutines
and is unsuitable for performance-sensitive code paths; some projects forbid it
entirely.

### Deferred-reply methods

`register_deferred_method<ReturnTypes...>()` registers a method whose handler
hands off to a callback-style asynchronous API and completes the D-Bus reply
later, without using a coroutine. The handler receives an
`sdbusplus::asio::deferred_reply<ReturnTypes...>` object by value, followed by
the decoded D-Bus input arguments.

The handler must call exactly one of `send(...)`, `send_errno(...)`, or
`send_error(...)` on the reply object. Any subsequent call on the same reply
object (or on one that has been moved-from) is silently ignored. If the reply
object is destroyed without a call, an automatic `-EIO` error reply is sent so
the caller does not time out.

```c++
iface->register_deferred_method<std::string>(
    "GetValue",
    [this](sdbusplus::asio::deferred_reply<std::string> reply,
           const std::string& key) {
        startAsyncLookup(
            key,
            [reply = std::move(reply)](boost::system::error_code ec,
                                       std::string value) mutable {
                if (ec)
                {
                    reply.send_errno(-EIO);
                    return;
                }
                reply.send(std::move(value));
            });
    });
```

For void methods, pass an empty template argument list:

```c++
iface->register_deferred_method<>(
    "Start",
    [this](sdbusplus::asio::deferred_reply<> reply) {
        startAsyncStart([reply = std::move(reply)](auto ec) mutable {
            if (ec)
            {
                reply.send_errno(-EIO);
                return;
            }
            reply.send();
        });
    });
```

`ReturnTypes...` are required because the handler always returns `void`, so the
D-Bus output signature cannot be deduced from the C++ return type.

`SD_BUS_VTABLE_METHOD_NO_REPLY` is unrelated: a deferred-reply method still
sends a reply, just later.

If the asynchronous completion may run on a thread other than the one driving
the D-Bus event loop, `boost::asio::post()` (or similar) the completion onto the
event-loop executor before calling `send()` on the reply object.
