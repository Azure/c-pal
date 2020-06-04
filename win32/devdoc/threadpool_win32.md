`threadpool_win32` requirements
================

## Overview

`threadpool_win32` is the Windows implementation of the threadpool interface.

It uses the PTP_POOL associated with the execution engine passed as argument to schedule the work.

## Exposed API

`threadpool_win32` implements the `threadpool` API:

```c
typedef struct THREADPOOL_TAG* THREADPOOL_HANDLE;

#define THREADPOOL_OPEN_RESULT_VALUES \
    THREADPOOL_OPEN_OK, \
    THREADPOOL_OPEN_ERROR

MU_DEFINE_ENUM(THREADPOOL_OPEN_RESULT, THREADPOOL_OPEN_RESULT_VALUES)

typedef void (*ON_THREADPOOL_OPEN_COMPLETE)(void* context, THREADPOOL_OPEN_RESULT open_result);
typedef void (*THREADPOOL_WORK_FUNCTION)(void* context);

MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
MOCKABLE_FUNCTION(, void, threadpool_destroy, THREADPOOL_HANDLE, threadpool);

MOCKABLE_FUNCTION(, int, threadpool_open_async, THREADPOOL_HANDLE, threadpool, ON_THREADPOOL_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
MOCKABLE_FUNCTION(, void, threadpool_close, THREADPOOL_HANDLE, threadpool);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THREADPOOL_HANDLE, threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

### threadpool_create

```c
MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`threadpool_create` creates a threadpool object that can execute work items.

**SRS_THREADPOOL_WIN32_01_001: [** `threadpool_create` shall allocate a new threadpool object and on success shall return a non-NULL handle. **]**

**SRS_THREADPOOL_WIN32_01_002: [** If `execution_engine` is NULL, `threadpool_create` shall fail and return NULL. **]**

**SRS_THREADPOOL_WIN32_01_025: [** `threadpool_create` shall obtain the PTP_POOL from the execution engine by calling `execution_engine_win32_get_threadpool`. **]**

**SRS_THREADPOOL_WIN32_01_003: [** If any error occurs, `threadpool_create` shall fail and return NULL. **]**

### threadpool_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_destroy, THREADPOOL_HANDLE, threadpool);
```

`threadpool_destroy` frees all resources associated with `threadpool`.

**SRS_THREADPOOL_WIN32_01_004: [** If `threadpool` is NULL, `threadpool_destroy` shall return. **]**

**SRS_THREADPOOL_WIN32_01_005: [** Otherwise, `threadpool_destroy` shall free all resources associated with `threadpool`. **]**

**SRS_THREADPOOL_WIN32_01_006: [** While `threadpool` is OPENING or CLOSING, `threadpool_destroy` shall wait for the open to complete either succesfully or with error. **]**

**SRS_THREADPOOL_WIN32_01_007: [** `threadpool_destroy` shall perform an implicit close if `threadpool` is OPEN. **]**

### threadpool_open_async

```c
MOCKABLE_FUNCTION(, int, threadpool_open_async, THREADPOOL_HANDLE, threadpool, ON_THREADPOOL_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`threadpool_open_async` opens the threadpool object so that subsequent calls to `threadpool_schedule_work` can be made.

**SRS_THREADPOOL_WIN32_01_008: [** If `threadpool` is NULL, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_009: [** If `on_open_complete` is NULL, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_010: [** `on_open_complete_context` shall be allowed to be NULL. **]**

**SRS_THREADPOOL_WIN32_01_011: [** Otherwise, `threadpool_open_async` shall switch the state to OPENING. **]**

**SRS_THREADPOOL_WIN32_01_026: [** `threadpool_open_async` shall initialize a thread pool environment by calling `InitializeThreadpoolEnvironment`. **]**

**SRS_THREADPOOL_WIN32_01_027: [** `threadpool_open_async` shall set the thread pool for the environment to the pool obtained from the execution engine by calling `SetThreadpoolCallbackPool`. **]**

**SRS_THREADPOOL_WIN32_01_028: [** `threadpool_open_async` shall create a threadpool cleanup group by calling `CreateThreadpoolCleanupGroup`. **]**

**SRS_THREADPOOL_WIN32_01_029: [** `threadpool_open_async` shall associate the cleanup group with the just created environment by calling `SetThreadpoolCallbackCleanupGroup`. **]**

**SRS_THREADPOOL_WIN32_01_015: [** `threadpool_open_async` shall set the state to OPEN. **]**

**SRS_THREADPOOL_WIN32_01_012: [** On success, `threadpool_open_async` shall return 0. **]**

**SRS_THREADPOOL_WIN32_01_014: [** On success, `threadpool_open_async` shall call `on_open_complete_context` shall with `THREADPOOL_OPEN_OK`. **]**

**SRS_THREADPOOL_WIN32_01_013: [** If `threadpool` is already OPEN or OPENING, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_040: [** If any error occurrs, `threadpool_open_async` shall fail and return a non-zero value. **]**

### threadpool_close

```c
MOCKABLE_FUNCTION(, void, threadpool_close, THREADPOOL_HANDLE, threadpool);
```

`threadpool_close` closes an open `threadpool`.

**SRS_THREADPOOL_WIN32_01_016: [** If `threadpool` is NULL, `threadpool_close` shall return. **]**

**SRS_THREADPOOL_WIN32_01_017: [** Otherwise, `threadpool_close` shall switch the state to CLOSING. **]**

**SRS_THREADPOOL_WIN32_01_030: [** `threadpool_close` shall wait for any executing callbacks by calling `CloseThreadpoolCleanupGroupMembers`, passing `FALSE` as `fCancelPendingCallbacks`. **]**

**SRS_THREADPOOL_WIN32_01_032: [** `threadpool_close` shall close the threadpool cleanup group by calling `CloseThreadpoolCleanupGroup`. **]**

**SRS_THREADPOOL_WIN32_01_033: [** `threadpool_close` shall destroy the thread pool environment created in `threadpool_open_async`. **]**

**SRS_THREADPOOL_WIN32_01_019: [** If `threadpool` is not OPEN, `threadpool_close` shall return. **]**

### threadpool_schedule_work

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THREADPOOL_HANDLE, threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_schedule_work` schedules a work item to be executed by the threadpool.

**SRS_THREADPOOL_WIN32_01_020: [** If `threadpool` is NULL, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_021: [** If `work_function` is NULL, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_022: [** `work_function_context` shall be allowed to be NULL. **]**

**SRS_THREADPOOL_WIN32_01_023: [** Otherwise `threadpool_schedule_work` shall allocate a context where `work_function` and `context` shall be saved. **]**

**SRS_THREADPOOL_WIN32_01_034: [** `threadpool_schedule_work` shall call `CreateThreadpoolWork` to schedule executiong the callback while apssing to it the `on_work_callback` function and the newly created context. **]**

**SRS_THREADPOOL_WIN32_01_041: [** `threadpool_schedule_work` shall call `SubmitThreadpoolWork` to submit the work item for execution. **]**

**SRS_THREADPOOL_WIN32_01_024: [** If any error occurs, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

### on_work_callback

```c
static VOID CALLBACK on_work_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work)
```

`on_work_callback` executes the work function passed to `threadpool_schedule_work`.

**SRS_THREADPOOL_WIN32_01_035: [** If `context` is NULL, `on_work_callback` shall return. **]**

**SRS_THREADPOOL_WIN32_01_036: [** Otherwise `context` shall be used as the context created in `threadpool_schedule_work`. **]**

**SRS_THREADPOOL_WIN32_01_037: [** The `work_function` callback passed to `threadpool_schedule_work` shall be called, passing to it the `work_function_context` argument passed to `threadpool_schedule_work`. **]**

**SRS_THREADPOOL_WIN32_01_038: [** `on_work_callback` shall call `CloseThreadpoolWork`. **]**

**SRS_THREADPOOL_WIN32_01_039: [** `on_work_callback` shall free the context allocated in `threadpool_schedule_work`. **]**
