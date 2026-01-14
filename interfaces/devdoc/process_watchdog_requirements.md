# process_watchdog requirements

## Overview

`process_watchdog` is a module that provides a simple watchdog timer for processes. When the watchdog timer expires, it calls `LogCriticalAndTerminate` to crash the process, which triggers Watson dump collection for debugging hung tests.

This module is thread-safe. The `process_watchdog_init` and `process_watchdog_deinit` functions use interlocked operations to ensure atomic initialization and deinitialization.

## Exposed API

```c
int process_watchdog_init(uint32_t timeout_ms);
void process_watchdog_deinit(void);
```

### process_watchdog_init

```c
int process_watchdog_init(uint32_t timeout_ms);
```

`process_watchdog_init` initializes and starts the watchdog timer with the specified timeout.

**SRS_PROCESS_WATCHDOG_43_001: [** `process_watchdog_init` shall call `interlocked_compare_exchange` to atomically check if the watchdog is already initialized. **]**

**SRS_PROCESS_WATCHDOG_43_002: [** If the watchdog is already initialized, `process_watchdog_init` shall fail and return a non-zero value. **]**

**SRS_PROCESS_WATCHDOG_43_003: [** `process_watchdog_init` shall create a timer that expires after `timeout_ms` milliseconds and calls `on_timer_expired`. **]**

**SRS_PROCESS_WATCHDOG_43_004: [** If creating the timer fails, `process_watchdog_init` shall call `interlocked_exchange` to atomically mark the watchdog as not initialized and return a non-zero value. **]**

**SRS_PROCESS_WATCHDOG_43_005: [** On success, `process_watchdog_init` shall return zero. **]**

### on_timer_expired

```c
static void on_timer_expired(void);
```

`on_timer_expired` is the internal callback invoked when the watchdog timer expires.

**SRS_PROCESS_WATCHDOG_43_006: [** `on_timer_expired` shall call `LogCriticalAndTerminate` to terminate the process. **]**

### process_watchdog_deinit

```c
void process_watchdog_deinit(void);
```

`process_watchdog_deinit` stops and cleans up the watchdog timer.

**SRS_PROCESS_WATCHDOG_43_007: [** `process_watchdog_deinit` shall call `interlocked_compare_exchange` to atomically check if the watchdog is initialized and mark it as not initialized. **]**

**SRS_PROCESS_WATCHDOG_43_008: [** If the watchdog is not initialized, `process_watchdog_deinit` shall return. **]**

**SRS_PROCESS_WATCHDOG_43_009: [** `process_watchdog_deinit` shall cancel and delete the timer. **]**
