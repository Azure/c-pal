# test_watchdog requirements

## Overview

`test_watchdog` is a module that provides a simple watchdog timer for test processes. When the watchdog timer expires, it calls `LogCriticalAndTerminate` to crash the process, which triggers Watson dump collection for debugging hung tests.

## Exposed API

```c
int test_watchdog_init(uint32_t timeout_ms);
void test_watchdog_deinit(void);
```

### test_watchdog_init

```c
int test_watchdog_init(uint32_t timeout_ms);
```

`test_watchdog_init` initializes and starts the watchdog timer with the specified timeout.

**SRS_TEST_WATCHDOG_01_001: [** If the watchdog is already initialized, `test_watchdog_init` shall fail and return a non-zero value. **]**

**SRS_TEST_WATCHDOG_01_002: [** `test_watchdog_init` shall create a timer that expires after `timeout_ms` milliseconds and calls `on_timer_expired`. **]**

**SRS_TEST_WATCHDOG_01_003: [** If creating the timer fails, `test_watchdog_init` shall fail and return a non-zero value. **]**

**SRS_TEST_WATCHDOG_01_004: [** On success, `test_watchdog_init` shall return zero. **]**

### on_timer_expired

```c
static void on_timer_expired(void);
```

`on_timer_expired` is the internal callback invoked when the watchdog timer expires.

**SRS_TEST_WATCHDOG_01_005: [** `on_timer_expired` shall call `LogCriticalAndTerminate` to terminate the process. **]**

### test_watchdog_deinit

```c
void test_watchdog_deinit(void);
```

`test_watchdog_deinit` stops and cleans up the watchdog timer.

**SRS_TEST_WATCHDOG_01_006: [** If the watchdog is not initialized, `test_watchdog_deinit` shall return. **]**

**SRS_TEST_WATCHDOG_01_007: [** `test_watchdog_deinit` shall cancel and delete the timer. **]**

**SRS_TEST_WATCHDOG_01_008: [** `test_watchdog_deinit` shall mark the watchdog as not initialized. **]**
