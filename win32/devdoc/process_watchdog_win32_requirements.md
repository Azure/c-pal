# process_watchdog_win32

## Overview

`process_watchdog_win32` provides the Windows implementation for the platform-independent `process_watchdog` module.

## References

[process_watchdog interface requirements](../../interfaces/devdoc/process_watchdog_requirements.md)

## Exposed API

See [process_watchdog interface requirements](../../interfaces/devdoc/process_watchdog_requirements.md) for the API documentation.

## Implementation Details

The Windows implementation uses the Windows Threadpool API to manage the watchdog timer.

### process_watchdog_init

**SRS_PROCESS_WATCHDOG_WIN32_43_001: [** `process_watchdog_init` shall call `CreateThreadpoolTimer` to create a timer with `on_timer_expired` as the callback. **]**

**SRS_PROCESS_WATCHDOG_WIN32_43_002: [** If `CreateThreadpoolTimer` fails, `process_watchdog_init` shall fail and return a non-zero value. **]**

**SRS_PROCESS_WATCHDOG_WIN32_43_003: [** `process_watchdog_init` shall call `SetThreadpoolTimer` to start the timer with the specified `timeout_ms`. **]**

### process_watchdog_deinit

**SRS_PROCESS_WATCHDOG_WIN32_43_004: [** `process_watchdog_deinit` shall call `SetThreadpoolTimer` with `NULL` to stop the timer. **]**

**SRS_PROCESS_WATCHDOG_WIN32_43_005: [** `process_watchdog_deinit` shall call `WaitForThreadpoolTimerCallbacks` to wait for any pending callbacks to complete. **]**

**SRS_PROCESS_WATCHDOG_WIN32_43_006: [** `process_watchdog_deinit` shall call `CloseThreadpoolTimer` to delete the timer. **]**
