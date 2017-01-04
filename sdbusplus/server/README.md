# transaction.hpp

Handles the generation and maintenance of the _transaction id_.

**What is _transaction id_** - A unique identifier created by hashing the bus
unique name and message cookie from a dbus call.

**When is _transaction_ generated** - Whenever the id is requested and has not
been initialized yet.

**Where is _transaction id_ stored** - In the running thread's local storage.

**How is _transaction id_ used** - Used to identify all the journal entries
associated with a dbus operation.

* When a journal entry is requested via the logging interface
([openbmc/phosphor-logging](https://github.com/openbmc/phosphor-logging)), it
adds the _transaction id_ as a metadata value. Therefore all journal entries
created within a thread would have the same _transaction id_ value.

* When an error/event log is requested, the _transaction id_ of that thread will
be used to collect the journal entries associated with that thread, providing
debug information for the complete operation.
