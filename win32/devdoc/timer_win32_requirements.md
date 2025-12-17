# timer_win32

## Overview

`timer_win32` provides the Windows implementation for the platform-independent timer module.

## References

[timer interface requirements](../../interfaces/devdoc/timer_requirements.md)

## Exposed API

See [timer interface requirements](../../interfaces/devdoc/timer_requirements.md) for the API documentation.

## Implementation Details

The Windows implementation uses `QueryPerformanceCounter` and `QueryPerformanceFrequency` to provide high-resolution timing.

### timer_create_new

**SRS_TIMER_WIN32_88_001: [** `timer_create_new` shall allocate memory for a new timer handle. **]**

**SRS_TIMER_WIN32_88_002: [** If memory allocation fails, `timer_create_new` shall return `NULL`. **]**

**SRS_TIMER_WIN32_88_003: [** `timer_create_new` shall call `QueryPerformanceFrequency` to obtain the timer frequency. **]**

**SRS_TIMER_WIN32_88_004: [** `timer_create_new` shall call `QueryPerformanceCounter` to record the start time. **]**

### timer_start

**SRS_TIMER_WIN32_88_005: [** `timer_start` shall call `QueryPerformanceCounter` to record the current time as the start time. **]**

### timer_get_elapsed

**SRS_TIMER_WIN32_88_006: [** `timer_get_elapsed` shall call `QueryPerformanceCounter` to get the current time. **]**

**SRS_TIMER_WIN32_88_007: [** `timer_get_elapsed` shall compute and return the elapsed time in seconds. **]**

### timer_get_elapsed_ms

**SRS_TIMER_WIN32_88_008: [** `timer_get_elapsed_ms` shall return the elapsed time in milliseconds by multiplying the result of `timer_get_elapsed` by 1000. **]**

### timer_destroy

**SRS_TIMER_WIN32_88_009: [** `timer_destroy` shall free the memory allocated for the timer handle. **]**

### timer_global_get_elapsed_s

**SRS_TIMER_WIN32_88_010: [** `timer_global_get_elapsed_s` shall call `QueryPerformanceFrequency` and `QueryPerformanceCounter` to compute the elapsed time in seconds. **]**

### timer_global_get_elapsed_ms

**SRS_TIMER_WIN32_88_011: [** `timer_global_get_elapsed_ms` shall call `QueryPerformanceFrequency` and `QueryPerformanceCounter` to compute the elapsed time in milliseconds. **]**

### timer_global_get_elapsed_us

**SRS_TIMER_WIN32_88_012: [** `timer_global_get_elapsed_us` shall call `QueryPerformanceFrequency` and `QueryPerformanceCounter` to compute the elapsed time in microseconds. **]**
