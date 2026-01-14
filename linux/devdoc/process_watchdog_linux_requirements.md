# process_watchdog_linux

## Overview

`process_watchdog_linux` provides the Linux implementation for the platform-independent `process_watchdog` module.

## References

[process_watchdog interface requirements](../../interfaces/devdoc/process_watchdog_requirements.md)

## Exposed API

See [process_watchdog interface requirements](../../interfaces/devdoc/process_watchdog_requirements.md) for the API documentation.

## Implementation Details

The Linux implementation uses `timer_create`, `timer_settime`, and `timer_delete` from the POSIX timer API to manage the watchdog timer. The timer uses `SIGEV_THREAD` notification to invoke the callback in a new thread when the timer expires.

### process_watchdog_init

**SRS_PROCESS_WATCHDOG_LINUX_43_001: [** `process_watchdog_init` shall call `timer_create` with `CLOCK_MONOTONIC` and `SIGEV_THREAD` notification to create a timer with `on_timer_expired` as the callback. **]**

**SRS_PROCESS_WATCHDOG_LINUX_43_002: [** If `timer_create` fails, `process_watchdog_init` shall fail and return a non-zero value. **]**

**SRS_PROCESS_WATCHDOG_LINUX_43_003: [** `process_watchdog_init` shall call `timer_settime` to start the timer with the specified `timeout_ms`. **]**

**SRS_PROCESS_WATCHDOG_LINUX_43_004: [** If `timer_settime` fails, `process_watchdog_init` shall call `timer_delete` to clean up and return a non-zero value. **]**

### process_watchdog_deinit

**SRS_PROCESS_WATCHDOG_LINUX_43_005: [** `process_watchdog_deinit` shall call `timer_delete` to stop and delete the timer. **]**
