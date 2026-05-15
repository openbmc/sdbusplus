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

### Completion methods

`register_completion_method<ReturnTypes...>()` registers a method whose handler
defers completion: rather than returning the result, the handler hands off to a
callback-style asynchronous API and completes the D-Bus method call later,
without using a coroutine. The handler receives an `sdbusplus::asio::completion`
by value, followed by the decoded D-Bus input arguments, and returns `void`.

`ReturnTypes...` describe the D-Bus output arguments. They are required because
the handler returns `void`, so the output signature cannot be deduced from the
C++ return type; pass an empty list for a void method.

The handler moves the `completion` into its asynchronous completion handler and
completes the call there, exactly once:

- `send(args...)` sends a successful method return, appending `args...` as the
  reply body.
- `message()` exposes the originating method-call message, so the handler can
  build any other reply with the usual `message_t` API — for example an error
  via `message().new_method_errno(error).method_return()`.

Completion is the handler's responsibility. Calling `send()` more than once, or
on a moved-from `completion`, is a no-op. A `completion` that is dropped without
being completed sends nothing, so the caller blocks until its own D-Bus timeout.

```c++
iface->register_completion_method<std::string>(
    "GetValue",
    [this](sdbusplus::asio::completion c, const std::string& key) {
        startAsyncLookup(
            key,
            [c = std::move(c)](boost::system::error_code ec,
                               std::string value) mutable {
                if (ec)
                {
                    c.message().new_method_errno(EIO).method_return();
                    return;
                }
                c.send(std::move(value));
            });
    });
```

If the asynchronous completion may run on a thread other than the one driving
the D-Bus event loop, `boost::asio::post()` (or similar) the completion onto the
event-loop executor before completing the `completion`.

A complete, runnable example is in
[`example/completion-method-example.cpp`](../example/completion-method-example.cpp),
which uses a timer to stand in for an asynchronous backend and shows the event
loop continuing to run while the reply is outstanding.
