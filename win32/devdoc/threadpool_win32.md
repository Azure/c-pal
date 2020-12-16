`threadpool_win32` requirements
================

## Overview

`threadpool_win32` is the Windows implementation of the [`threadpool`](../../interfaces/devdoc/threadpool.md) interface.

It uses the PTP_POOL associated with the execution engine passed as argument to schedule the work.

## Exposed API

`threadpool_win32` implements the `threadpool` API:

```c
typedef struct THREADPOOL_TAG* THREADPOOL_HANDLE;
typedef struct TIMER_INSTANCE_TAG* TIMER_INSTANCE_HANDLE;

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

MOCKABLE_FUNCTION(, int, threadpool_timer_start, THREADPOOL_HANDLE, threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);

MOCKABLE_FUNCTION(, int, threadpool_timer_restart, TIMER_INSTANCE_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);

MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, TIMER_INSTANCE_HANDLE, timer);

MOCKABLE_FUNCTION(, void, threadpool_timer_destroy, TIMER_INSTANCE_HANDLE, timer);
```

### threadpool_create

```c
MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`threadpool_create` creates a threadpool object that can execute work items.

**SRS_THREADPOOL_WIN32_01_001: [** `threadpool_create` shall allocate a new threadpool object and on success shall return a non-`NULL` handle. **]**

**SRS_THREADPOOL_WIN32_01_002: [** If `execution_engine` is `NULL`, `threadpool_create` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_WIN32_01_025: [** `threadpool_create` shall obtain the PTP_POOL from the execution engine by calling `execution_engine_win32_get_threadpool`. **]**

**SRS_THREADPOOL_WIN32_01_003: [** If any error occurs, `threadpool_create` shall fail and return `NULL`. **]**

### threadpool_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_destroy, THREADPOOL_HANDLE, threadpool);
```

`threadpool_destroy` frees all resources associated with `threadpool`.

**SRS_THREADPOOL_WIN32_01_004: [** If `threadpool` is `NULL`, `threadpool_destroy` shall return. **]**

**SRS_THREADPOOL_WIN32_01_005: [** Otherwise, `threadpool_destroy` shall free all resources associated with `threadpool`. **]**

**SRS_THREADPOOL_WIN32_01_006: [** While `threadpool` is OPENING or CLOSING, `threadpool_destroy` shall wait for the open to complete either successfully or with error. **]**

**SRS_THREADPOOL_WIN32_01_007: [** `threadpool_destroy` shall perform an implicit close if `threadpool` is OPEN. **]**

### threadpool_open_async

```c
MOCKABLE_FUNCTION(, int, threadpool_open_async, THREADPOOL_HANDLE, threadpool, ON_THREADPOOL_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`threadpool_open_async` opens the threadpool object so that subsequent calls to `threadpool_schedule_work` can be made.

**SRS_THREADPOOL_WIN32_01_008: [** If `threadpool` is `NULL`, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_009: [** If `on_open_complete` is `NULL`, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_010: [** `on_open_complete_context` shall be allowed to be `NULL`. **]**

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

**SRS_THREADPOOL_WIN32_01_016: [** If `threadpool` is `NULL`, `threadpool_close` shall return. **]**

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

**SRS_THREADPOOL_WIN32_01_020: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_021: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_01_022: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_WIN32_01_023: [** Otherwise `threadpool_schedule_work` shall allocate a context where `work_function` and `context` shall be saved. **]**

**SRS_THREADPOOL_WIN32_01_034: [** `threadpool_schedule_work` shall call `CreateThreadpoolWork` to schedule execution the callback while passing to it the `on_work_callback` function and the newly created context. **]**

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

### threadpool_timer_start

```c
MOCKABLE_FUNCTION(, int, threadpool_timer_start, THREADPOOL_HANDLE, threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);
```

`threadpool_timer_start` starts a threadpool timer which runs after `start_delay_ms` milliseconds and then runs again every `timer_period_ms` milliseconds until `threadpool_timer_cancel` or `threadpool_timer_destroy` is called. The `timer_handle` must be stopped before closing/destroying the threadpool.

**SRS_THREADPOOL_WIN32_42_001: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_42_002: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_42_003: [** If `timer_handle` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_42_004: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_WIN32_42_005: [** `threadpool_timer_start` shall allocate a context for the timer being started and store `work_function` and `work_function_context` in it. **]**

**SRS_THREADPOOL_WIN32_42_006: [** `threadpool_timer_start` shall call `CreateThreadpoolTimer ` to schedule execution the callback while passing to it the `on_timer_callback` function and the newly created context. **]**

**SRS_THREADPOOL_WIN32_42_007: [** `threadpool_timer_start` shall call `SetThreadpoolTimer`, passing negative `start_delay_ms` as `pftDueTime`, `timer_period_ms` as `msPeriod`, and 0 as `msWindowLength`. **]**

**SRS_THREADPOOL_WIN32_42_008: [** If any error occurs, `threadpool_timer_start` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_42_009: [** `threadpool_timer_start` shall return the allocated handle in `timer_handle`. **]**

**SRS_THREADPOOL_WIN32_42_010: [** `threadpool_timer_start` shall succeed and return 0. **]**

### threadpool_timer_restart

```c
MOCKABLE_FUNCTION(, int, threadpool_timer_restart, TIMER_INSTANCE_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`threadpool_timer_restart` changes the delay and period of an existing timer.

**SRS_THREADPOOL_WIN32_42_019: [** If `timer` is `NULL`, `threadpool_timer_restart` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_42_022: [** `threadpool_timer_restart` shall call `SetThreadpoolTimer`, passing negative `start_delay_ms` as `pftDueTime`, `timer_period_ms` as `msPeriod`, and 0 as `msWindowLength`. **]**

**SRS_THREADPOOL_WIN32_42_023: [** `threadpool_timer_restart` shall succeed and return 0. **]**

### threadpool_timer_cancel

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, TIMER_INSTANCE_HANDLE, timer);
```

`threadpool_timer_cancel` stops the timer and waits for any pending callbacks. Afterward, the timer may be resumed with a new time by calling `threadpool_timer_restart` or cleaned up by calling `threadpool_timer_destroy`.

**SRS_THREADPOOL_WIN32_42_024: [** If `timer` is `NULL`, `threadpool_timer_cancel` shall fail and return. **]**

**SRS_THREADPOOL_WIN32_42_025: [** `threadpool_timer_cancel` shall call `SetThreadpoolTimer` with `NULL` for `pftDueTime` and 0 for `msPeriod` and `msWindowLength` to cancel ongoing timers. **]**

**SRS_THREADPOOL_WIN32_42_026: [** `threadpool_timer_cancel` shall call `WaitForThreadpoolTimerCallbacks`. **]**

### threadpool_timer_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_destroy, TIMER_INSTANCE_HANDLE, timer);
```

`threadpool_timer_destroy` stops the timer started by `threadpool_timer_start` and cleans up its resources.

**SRS_THREADPOOL_WIN32_42_011: [** If `timer` is `NULL`, `threadpool_timer_destroy` shall fail and return. **]**

**SRS_THREADPOOL_WIN32_42_012: [** `threadpool_timer_destroy` shall call `SetThreadpoolTimer` with `NULL` for `pftDueTime` and 0 for `msPeriod` and `msWindowLength` to cancel ongoing timers. **]**

**SRS_THREADPOOL_WIN32_42_013: [** `threadpool_timer_destroy` shall call `WaitForThreadpoolTimerCallbacks`. **]**

**SRS_THREADPOOL_WIN32_42_014: [** `threadpool_timer_destroy` shall call `CloseThreadpoolTimer`. **]**

**SRS_THREADPOOL_WIN32_42_015: [** `threadpool_timer_destroy` shall free all resources in `timer`. **]**

### on_timer_callback

```c
static VOID CALLBACK on_timer_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer);
```

`on_work_callback` executes the work function passed to `threadpool_schedule_work`.

**SRS_THREADPOOL_WIN32_42_016: [** If `context` is `NULL`, `on_work_callback` shall return. **]**

**SRS_THREADPOOL_WIN32_42_017: [** Otherwise `context` shall be used as the context created in `threadpool_schedule_work`. **]**

**SRS_THREADPOOL_WIN32_42_018: [** The `work_function` callback passed to `threadpool_schedule_work` shall be called, passing to it the `work_function_context` argument passed to `threadpool_schedule_work`. **]**
