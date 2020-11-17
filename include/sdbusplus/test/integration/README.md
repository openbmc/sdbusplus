# D-Bus Mocking for Integration Testing

This directory (and its src counterpart) includes the infrastructure to support
mocking D-Bus services for integration testing as discussed in
[gerrit/37378](https://gerrit.openbmc-project.xyz/c/openbmc/docs/+/37378).

## Files

- daemon_manager
	- Manages a daemon that should be run externally.
	- Accepts an argv list and can be optionally provided with alternative file
    paths for stdout and stderr.
	- The daemon is started after returning from a call to `start()` and stopped
    upon object destruction.	
	- If stdout path is provided, the daemon output can be captured by calling
    `captureStdoutLines()`. 
	- Daemons extending this class can implement `postStart()`/`postTerminate()`
    for customized activities after starting/terminating a daemon, but before
    returning.
- dbusd_manager
	- Manages dbus-daemon.
	- Accepts the configuration path for D-Bus and the file path to forward
    stdout.
- mock_object
	- A class for including functionalities shared by mock objects implementing
    D-Bus interfaces.
    - Services should call start on the object, when they have
    their bus connection.
- mock_service
	- A class for including functionalities shared by mock services with mock
    objects that implement D-Bus interfaces.
	- Each service has a worker thread that starts when the user calls `start()`
    on the service.
    - The worker thread executes the `run` function, that gets a bus connection,
    registers on it, and starts timed loop that waits for an event on bus and
    calls `proceed()`.
    - Active services can override the `proceed()` function to customize the
    thread behavior.
    Mock services that are not active and only react to events such as a
    D-Bus signal, do not need to override `proceed()`.
- private_bus
	- This is the class that should be used by the user to start the bus.
    It provides an exclusive bus per test to enable parallel tests and hides
    the details of resource management.
	- The bus daemon starts upon construction and terminates upon destruction.
    - Each service should call registerService function to get the bus
    connection and the well-known name.
    - Each service should register from its own thread.
- test_timer
	- This class is used in loop-based procedures that are run for a certain
    amount of time.

## Examples
Examples of using these classes for integration testing is added to the
phosphor-pid-control repository.
Adding more examples is the work TBD.
