# timer
================

## Overview

`timer` provides platform-independent time measurement primitives.

## Exposed API

```c
typedef struct TIMER_HANDLE_DATA_TAG* TIMER_HANDLE;

MOCKABLE_FUNCTION(, TIMER_HANDLE, timer_create_new);
MOCKABLE_FUNCTION(, int, timer_start, TIMER_HANDLE, handle);
MOCKABLE_FUNCTION(, double, timer_get_elapsed, TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, double, timer_get_elapsed_ms, TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, void, timer_destroy, TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed_ms);
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed_us);
```

### timer_create_new

```c
MOCKABLE_FUNCTION(, TIMER_HANDLE, timer_create_new);
```

`timer_create_new` creates a new timer.

**SRS_TIMER_01_001: [** `timer_create_new` shall create a new timer and on success return a non-`NULL` handle to it. **]**

### timer_destroy

```c
MOCKABLE_FUNCTION(, void, timer_destroy, TIMER_HANDLE, timer);
```

`timer_destroy` frees all resources associated with `timer`.

**SRS_TIMER_01_002: [** If `timer` is `NULL`, `timer_destroy` shall return. **]**

**SRS_TIMER_01_003: [** Otherwise, `timer_destroy` shall free the memory associated with `timer`. **]**

### timer_start

```c
MOCKABLE_FUNCTION(, int, timer_start, TIMER_HANDLE, timer);
```

`timer_start` starts the timer.

**SRS_TIMER_01_004: [** If `timer` is `NULL`, `timer_start` shall fail and return a non-zero value. **]**

**SRS_TIMER_01_005: [** Otherwise, `timer_start` shall record the start time (used for computing the elapsed time). **]**

### timer_get_elapsed

```c
MOCKABLE_FUNCTION(, double, timer_get_elapsed, TIMER_HANDLE, timer);
```

`timer_get_elapsed` returns the time in seconds elapsed since the timer was started.

**SRS_TIMER_01_006: [** If `timer` is `NULL`, `timer_get_elapsed` shall return -1. **]**

**SRS_TIMER_01_007: [** Otherwise `timer_get_elapsed` shall return the time difference in seconds between the current time and the start time of the timer. **]**

### timer_get_elapsed_ms

```c
MOCKABLE_FUNCTION(, double, timer_get_elapsed_ms, TIMER_HANDLE, timer);
```

`timer_get_elapsed_ms` returns the time in milliseconds elapsed since the timer was started.

**SRS_TIMER_01_008: [** if `timer` is `NULL`, `timer_get_elapsed_ms` shall return -1. **]**

**SRS_TIMER_01_009: [** Otherwise `timer_get_elapsed_ms` shall return the time difference in milliseconds between the current time and the start time of the timer. **]**

### timer_global_get_elapsed_ms

```c
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed_ms);
```

`timer_global_get_elapsed_ms` returns the elapsed time in milliseconds from a start time in the past (the actual point in time is unspecified).

**SRS_TIMER_01_010: [** `timer_global_get_elapsed_ms` shall return the elapsed time in milliseconds from a start time in the past. **]**

### timer_global_get_elapsed_us

```c
MOCKABLE_FUNCTION(, double, timer_global_get_elapsed_us);
```

`timer_global_get_elapsed_us` returns the elapsed time in microseconds from a start time in the past (the actual point in time is unspecified).

**SRS_TIMER_01_011: [** `timer_global_get_elapsed_us` shall return the elapsed time in microseconds from a start time in the past. **]**
