# transaction.hpp

Handles the generation and maintenance of the _transaction id_.

**What is _transaction id_** - A unique identifier created by hashing the bus
name and message cookie from a dbus call. The bus name (unique to each
application) and message cookie (unique within each bus peer) allows each dbus
message to be uniquely identified.

**When is _transaction id_ generated** - When an error response message is
created, and whenever the id is requested and has not been initialized yet.

**Where is _transaction id_ stored** - In the exception object when an
application error occurs.

**How is _transaction id_ used** - Used to identify all the journal entries
associated with a dbus operation.

- When a journal entry is created, the _transaction id_ is added as metadata.
  Therefore all journal entries created within a message have the same
  _transaction id_ value.

- When an error/event log is created, the _transaction id_ of that message will
  be used to collect the journal entries associated with that message, providing
  debug information for the complete operation.

**Multiple _transaction ids_** - The exception object can store multiple
_transaction ids_ for operations that create multiple messages.
