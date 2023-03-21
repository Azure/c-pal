# `execution_engine_linux` requirements


## Overview

`execution_engine_linux` is a module that implements the execution engine used for creating a `threadpool`.

## Design

`execution_engine_linux` is a linux implemented version of execution engine in order to keep same with win32. It contains a ref counted `EXECUTION_ENGINE` type which specifies the min and max thread count .

## Exposed API

`execution_engine_linux` implements the `execution_engine` API and additionally exposes the following API:

```c
    typedef struct EXECUTION_ENGINE_PARAMETERS_LINUX_TAG
    {
        uint32_t min_thread_count;
        uint32_t max_thread_count;
    } EXECUTION_ENGINE_PARAMETERS_LINUX;

#define DEFAULT_MIN_THREAD_COUNT 4
#define DEFAULT_MAX_THREAD_COUNT 0 // no max thread count

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, void*, execution_engine_parameters);
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS_LINUX*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);
```

### execution_engine_create

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, void*, execution_engine_parameters);
```

`execution_engine_create` creates an execution engine.

**SRS_EXECUTION_ENGINE_LINUX_07_001: [** `execution_engine_create` shall allocate a new execution engine and on success shall return a non-NULL handle. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_002: [** If `execution_engine_parameters` is NULL, `execution_engine_create` shall use the default `DEFAULT_MIN_THREAD_COUNT` and `DEFAULT_MAX_THREAD_COUNT` as parameters. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_003: [** `execution_engine_create` shall set the minimum number of threads to the `min_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_004: [** `execution_engine_create` shall set the maximum number of threads to the `max_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_005: [** If `max_thread_count` is non-zero and less than `min_thread_count`, `execution_engine_create` shall fail and return NULL. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_006: [** If any error occurs, `execution_engine_create` shall fail and return NULL. **]**

### execution_engine_dec_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_dec_ref` decrements the ref count and frees all resources associated with `execution_engine` if needed.

**SRS_EXECUTION_ENGINE_LINUX_07_007: [** If `execution_engine` is NULL, `execution_engine_dec_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_008: [** Otherwise `execution_engine_dec_ref` shall decrement the refcount. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_009: [** If the refcount is zero `execution_engine_dec_ref` shall free the memory for `EXECUTION_ENGINE`. **]**

### execution_engine_inc_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_inc_ref` increments the ref count on the `execution_engine`.

**SRS_EXECUTION_ENGINE_LINUX_07_010: [** If `execution_engine` is `NULL` then `execution_engine_inc_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_011: [** Otherwise `execution_engine_inc_ref` shall increment the reference count for `execution_engine`. **]**


### execution_engine_linux_get_parameters

```c
MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS_LINUX*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_linux_get_parameters` returns the `EXECUTION_ENGINE_PARAMETERS_LINUX` handle.

**SRS_EXECUTION_ENGINE_LINUX_07_012: [** If `execution_engine` is NULL, `execution_engine_linux_get_parameters` shall fail and return NULL. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_013: [** Otherwise, `execution_engine_linux_get_parameters` shall return the parameters in `EXECUTION_ENGINE`. **]**
