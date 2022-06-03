# ASIO Set Property callback

When registering a writable property using ASIO, the set callback should meet the
following prototype:

```c++
bool setHandler(const PropertyType& newValue, PropertyType& value);
```

The first argument is the new value requested to be set by the D-Bus caller.

The second argument is the actual underlying value contained within the object
server. If the new value meets the expected constraints, the handler must set
`value = newValue` to make the set operation take effect and return true. If the
new value is invalid or cannot be applied for whatever reason, the handler must
leave `value` unmodified and return false.
