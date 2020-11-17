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
- mock_service
	- A class for including functionalities shared by mock services with mock
    objects that implement D-Bus interfaces.
	- Mock services that are not active and only react to events such as a
    D-Bus signal, should inherit `MockService`.
	- Mock services that are active and should run a procedure during the test
    execution should inherit `MockServiceActive`.
	- Children of this class can override `run()` and `proceed()` methods.
		- In the default implementation, a thread runs the `run()` function,
        which is a timed loop that calls  `proceed()`. In most cases,
        this should be sufficient and children should only implement the
        `proceed()` function, which is the actual work that needs to be done.
- private_bus
	- This is the class that should be used by the user to start the bus.
    It provides an exclusive bus per test to enable parallel tests and hides
    the details of resource management.
	- The bus starts upon construction and terminates upon destruction.
- test_timer
	- This class is used in loop-based procedures that are run for a certain
    amount of time.

## Examples
Examples of using these classes for integration testing is added to the
phosphor-pid-control repository.
Adding more examples is the work TBD.
