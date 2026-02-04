# timed_test_suite requirements

## Overview

`timed_test_suite` provides wrapper macros for test suite initialization and cleanup that integrate `process_watchdog` to provide automatic timeout protection for integration tests that may hang.

## References

- `process_watchdog`
- `testrunnerswitcher`

## Exposed API

```c
#define TIMED_TEST_DEFAULT_TIMEOUT_MS 600000

#define TIMED_TEST_SUITE_INITIALIZE(name, timeout_ms, ...)

#define TIMED_TEST_SUITE_CLEANUP(name, ...)
```

### TIMED_TEST_SUITE_INITIALIZE

```c
#define TIMED_TEST_SUITE_INITIALIZE(name, timeout_ms, ...)
```

`TIMED_TEST_SUITE_INITIALIZE` creates a test suite initialization function that starts the process watchdog before executing user initialization code.

**SRS_TIMED_TEST_SUITE_43_002: [** `TIMED_TEST_SUITE_INITIALIZE` shall create a static fixture function that calls `process_watchdog_init` with `timeout_ms`. **]**

**SRS_TIMED_TEST_SUITE_43_003: [** `TIMED_TEST_SUITE_INITIALIZE` shall call `TEST_SUITE_INITIALIZE` with the watchdog init fixture as the first fixture, followed by any additional fixtures passed via variadic arguments. **]**

**SRS_TIMED_TEST_SUITE_43_004: [** The watchdog init fixture shall execute before the user's initialization code. **]**

### TIMED_TEST_SUITE_CLEANUP

```c
#define TIMED_TEST_SUITE_CLEANUP(name, ...)
```

`TIMED_TEST_SUITE_CLEANUP` creates a test suite cleanup function that stops the process watchdog after executing user cleanup code.

**SRS_TIMED_TEST_SUITE_43_005: [** `TIMED_TEST_SUITE_CLEANUP` shall create a static fixture function that calls `process_watchdog_deinit`. **]**

**SRS_TIMED_TEST_SUITE_43_006: [** `TIMED_TEST_SUITE_CLEANUP` shall call `TEST_SUITE_CLEANUP` with any additional fixtures passed via variadic arguments, followed by the watchdog deinit fixture as the last fixture. **]**

**SRS_TIMED_TEST_SUITE_43_007: [** The watchdog deinit fixture shall execute after the user's cleanup code. **]**
