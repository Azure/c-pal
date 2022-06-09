# `execution_engine` interface requirements

## Overview

`execution_engine` is an interface that allows creating an execution engine for a given platform (Windows, Linux, etc.).

An execution engine is the context needed for being able to create a threadpool, an asynchronous socket API or an asynchronous file API.

## Exposed API

```c
typedef struct EXECUTION_ENGINE* EXECUTION_ENGINE_HANDLE;

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

**SRS_EXECUTION_ENGINE_01_001: [** `execution_engine_create` shall allocate a new execution engine and on success shall return a non-NULL handle. **]**

**SRS_EXECUTION_ENGINE_01_002: [** `execution_engine_parameters` shall be interpreted depending on the specific platform implementation. **]**

**SRS_EXECUTION_ENGINE_01_003: [** If any error occurs, `execution_engine_create` shall fail and return NULL. **]**

### execution_engine_dec_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_destroy, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_destroy` frees all resources associated with `execution_engine`.

**SRS_EXECUTION_ENGINE_01_004: [** If `execution_engine` is NULL, `execution_engine_destroy` shall return. **]**

**SRS_EXECUTION_ENGINE_03_001: [** Otherwise `execution_engine_dec_ref` shall decrement the refcount. **]**

**SRS_EXECUTION_ENGINE_03_002: [** If the refcount is zero `execution_engine_dec_ref` shall close the threadpool. **]**

```c
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_inc_ref` increments the ref count on the `execution_engine`.

**SRS_EXECUTION_ENGINE_03_003: [** If `execution_engine` is `NULL` then `execution_engine_inc_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_03_004: [** Otherwise `execution_engine_inc_ref` shall increment the reference count for `execution_engine`. **]**
