`execution_engine_win32` requirements
================

## Overview

`execution_engine_win32` is a module that implements the execution engine supporting threadpool, asynchronous socket APIs and asynchronous file APIs for Windows.

## Design

`execution_engine_win32` is backed by a Win32 threadpool (PTP_POOL).

## Exposed API

`execution_engine_win32` implements the `execution_engine` API and additionally exposes the following API:

```c
    typedef struct EXECUTION_ENGINE_PARAMETERS_WIN32_TAG
    {
        uint32_t min_thread_count;
        uint32_t max_thread_count;
    } EXECUTION_ENGINE_PARAMETERS_WIN32;

#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0 // no max thread count

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, void*, execution_engine_parameters);
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, PTP_POOL, execution_engine_win32_get_threadpool, EXECUTION_ENGINE_HANDLE, execution_engine);
```

### execution_engine_create

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, void*, execution_engine_parameters);
```

`execution_engine_create` creates an execution engine.

**SRS_EXECUTION_ENGINE_WIN32_01_001: [** `execution_engine_create` shall allocate a new execution engine and on success shall return a non-NULL handle. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_011: [** If `execution_engine_parameters` is NULL, `execution_engine_create` shall use the defaults `DEFAULT_MIN_THREAD_COUNT` and `DEFAULT_MAX_THREAD_COUNT` as parameters. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_002: [** `execution_engine_parameters` shall be interpreted as `EXECUTION_ENGINE_PARAMETERS_WIN32`. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_003: [** `execution_engine_create` shall call `CreateThreadpool` to create the Win32 threadpool. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_004: [** `execution_engine_create` shall set the minimum number of threads to the `min_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_005: [** `execution_engine_create` shall set the maximum number of threads to the `max_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_012: [** If `max_thread_count` is 0, `execution_engine_create` shall not set the maximum thread count. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_013: [** If `max_thread_count` is non-zero, but less than `min_thread_count`, `execution_engine_create` shall fail and return NULL. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_006: [** If any error occurs, `execution_engine_create` shall fail and return NULL. **]**

### execution_engine_dec_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_dec_ref` decrements the ref count and frees all resources associated with `execution_engine` if needed.

**SRS_EXECUTION_ENGINE_WIN32_01_007: [** If `execution_engine` is NULL, `execution_engine_dec_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_WIN32_03_001: [** Otherwise `execution_engine_dec_ref` shall decrement the refcount. **]**

**SRS_EXECUTION_ENGINE_WIN32_03_002: [** If the refcount is zero `execution_engine_dec_ref` shall close the threadpool. **]**

```c
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_inc_ref` increments the ref count on the `execution_engine`.

**SRS_EXECUTION_ENGINE_WIN32_03_003: [** If `execution_engine` is `NULL` then `execution_engine_inc_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_WIN32_03_004: [** Otherwise `execution_engine_inc_ref` shall increment the reference count for `execution_engine`. **]**


### execution_engine_win32_get_threadpool

```c
MOCKABLE_FUNCTION(, PTP_POOL, execution_engine_win32_get_threadpool, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_win32_get_threadpool` returns the underlying Win32 threadpool handle.

**SRS_EXECUTION_ENGINE_WIN32_01_009: [** If `execution_engine` is NULL, `execution_engine_win32_get_threadpool` shall fail and return NULL. **]**

**SRS_EXECUTION_ENGINE_WIN32_01_010: [** Otherwise, `execution_engine_win32_get_threadpool` shall return the threadpool handle created in `execution_engine_create`. **]**
