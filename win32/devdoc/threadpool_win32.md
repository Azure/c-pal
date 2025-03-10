﻿# `threadpool_win32` requirements

## Overview

`threadpool_win32` is the Windows implementation of the [`threadpool`](../../interfaces/devdoc/threadpool.md) interface.

It uses the PTP_POOL associated with the execution engine passed as argument to schedule the work.

## Exposed API

`threadpool_win32` implements the `threadpool` API:

```c
typedef struct THREADPOOL_TAG THREADPOOL;
typedef struct THREADPOOL_WORK_ITEM_TAG THREADPOOL_WORK_ITEM;
typedef void (*THREADPOOL_WORK_FUNCTION)(void* context);

typedef struct THREADPOOL_TIMER_TAG THREADPOOL_TIMER;
THANDLE_TYPE_DECLARE(THREADPOOL_TIMER);

THANDLE_TYPE_DECLARE(THREADPOOL);
THANDLE_TYPE_DECLARE(THREADPOOL_WORK_ITEM);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_WORK_ITEM), threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THANDLE(THREADPOOL_WORK_ITEM), threadpool_work_item);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_TIMER), threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_timer_restart, THANDLE(THREADPOOL_TIMER), timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);

MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, THANDLE(THREADPOOL_TIMER), timer);
```

### threadpool_create

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`threadpool_create` creates a threadpool object that can execute work items.

**SRS_THREADPOOL_WIN32_01_001: [** `threadpool_create` shall allocate a new threadpool object and on success shall return a non-`NULL` handle. **]**

**SRS_THREADPOOL_WIN32_01_002: [** If `execution_engine` is `NULL`, `threadpool_create` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_WIN32_42_027: [** `threadpool_create` shall increment the reference count on the `execution_engine`. **]**

**SRS_THREADPOOL_WIN32_01_025: [** `threadpool_create` shall obtain the PTP_POOL from the execution engine by calling `execution_engine_win32_get_threadpool`. **]**

**SRS_THREADPOOL_WIN32_01_026: [** `threadpool_create` shall initialize a thread pool environment by calling `InitializeThreadpoolEnvironment`. **]**

**SRS_THREADPOOL_WIN32_01_027: [** `threadpool_create` shall set the thread pool for the environment to the pool obtained from the execution engine by calling `SetThreadpoolCallbackPool`. **]**

**SRS_THREADPOOL_WIN32_01_028: [** `threadpool_create` shall create a threadpool cleanup group by calling `CreateThreadpoolCleanupGroup`. **]**

**SRS_THREADPOOL_WIN32_01_029: [** `threadpool_create` shall associate the cleanup group with the just created environment by calling `SetThreadpoolCallbackCleanupGroup`. **]**

**SRS_THREADPOOL_WIN32_01_003: [** If any error occurs, `threadpool_create` shall fail and return `NULL`. **]**

### threadpool_dispose

```C
static void threadpool_dispose(THREADPOOL* threadpool)
```

`threadpool_dispose` frees all resources associated with threadpool.

**SRS_THREADPOOL_WIN32_01_030: [** `threadpool_dispose` shall wait for any executing callbacks by calling `CloseThreadpoolCleanupGroupMembers`, passing `FALSE` as `fCancelPendingCallbacks`. **]**

**SRS_THREADPOOL_WIN32_01_032: [** `threadpool_dispose` shall close the threadpool cleanup group by calling `CloseThreadpoolCleanupGroup`. **]**

**SRS_THREADPOOL_WIN32_01_033: [** `threadpool_dispose` shall destroy the thread pool environment created in `threadpool_create`. **]**

**SRS_THREADPOOL_WIN32_42_028: [** `threadpool_dispose` shall decrement the reference count on the `execution_engine`. **]**


### threadpool_schedule_work

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
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

### on_work_callback_v2

```c
static VOID CALLBACK on_work_callback_v2(PTP_CALLBACK_INSTANCE instance, void* context, PTP_WORK work);
```

`on_work_callback_v2` executes the work function passed to `threadpool_create_work_item`.

**SRS_THREADPOOL_WIN32_05_001: [** If `context` is `NULL`, `on_work_callback_v2` shall Log Message with severity `CRITICAL` and `terminate`. **]**

**SRS_THREADPOOL_WIN32_05_002: [** Otherwise `context` shall be used as the context created in `threadpool_create_work_item`. **]**

**SRS_THREADPOOL_WIN32_05_003: [** The `work_function` callback passed to `threadpool_create_work_item` shall be called with the `work_function_context` as an argument. `work_function_context` was set inside the `threadpool_create_work_item` as an argument to `CreateThreadpoolContext`. **]**


### threadpool_timer_start

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_TIMER), threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_timer_start` starts a threadpool timer which runs after `start_delay_ms` milliseconds and then runs again every `timer_period_ms` milliseconds until `threadpool_timer_cancel` or `threadpool_timer_destroy` is called. The `timer_handle` must be stopped before closing/destroying the threadpool.

**SRS_THREADPOOL_WIN32_42_001: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return NULL. **]**

**SRS_THREADPOOL_WIN32_42_002: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return NULL. **]**

**SRS_THREADPOOL_WIN32_42_004: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_WIN32_42_005: [** `threadpool_timer_start` shall allocate memory for `THANDLE(THREADPOOL_TIMER)`, passing `threadpool_timer_dispose` as dispose function, and store `work_function` and `work_function_context` in it. **]**

**SRS_THREADPOOL_WIN32_42_006: [** `threadpool_timer_start` shall call `CreateThreadpoolTimer` to schedule execution the callback while passing to it the `on_timer_callback` function and the newly created context. **]**

**SRS_THREADPOOL_WIN32_07_002: [** `threadpool_timer_start` shall initialize the lock guarding the timer state by calling `srw_lock_ll_init`. **]**

**SRS_THREADPOOL_WIN32_42_007: [** `threadpool_timer_start` shall call `SetThreadpoolTimer`, passing negative `start_delay_ms` as `pftDueTime`, `timer_period_ms` as `msPeriod`, and 0 as `msWindowLength`. **]**

**SRS_THREADPOOL_WIN32_42_008: [** If any error occurs, `threadpool_timer_start` shall fail and return NULL. **]**

**SRS_THREADPOOL_WIN32_42_010: [** `threadpool_timer_start` shall succeed and return a non-NULL handle. **]**

### threadpool_timer_restart

```c
MOCKABLE_FUNCTION(, int, threadpool_timer_restart, THANDLE(THREADPOOL_TIMER), timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`threadpool_timer_restart` changes the delay and period of an existing timer.

**SRS_THREADPOOL_WIN32_42_019: [** If `timer` is `NULL`, `threadpool_timer_restart` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_07_003: [** `threadpool_timer_restart` shall acquire an exclusive lock. **]**

**SRS_THREADPOOL_WIN32_42_022: [** `threadpool_timer_restart` shall call `SetThreadpoolTimer`, passing negative `start_delay_ms` as `pftDueTime`, `timer_period_ms` as `msPeriod`, and 0 as `msWindowLength`. **]**

**SRS_THREADPOOL_WIN32_07_004: [** `threadpool_timer_restart` shall release the exclusive lock.  **]**

**SRS_THREADPOOL_WIN32_42_023: [** `threadpool_timer_restart` shall succeed and return 0. **]**

### threadpool_timer_cancel

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, THANDLE(THREADPOOL_TIMER), timer);
```

`threadpool_timer_cancel` stops the timer and waits for any pending callbacks. Afterward, the timer may be resumed with a new time by calling `threadpool_timer_restart` or cleaned up by calling `threadpool_timer_destroy`.

**SRS_THREADPOOL_WIN32_42_024: [** If `timer` is `NULL`, `threadpool_timer_cancel` shall fail and return. **]**

**SRS_THREADPOOL_WIN32_07_005: [** `threadpool_timer_cancel` shall acquire an exclusive lock. **]**

**SRS_THREADPOOL_WIN32_42_025: [** `threadpool_timer_cancel` shall call `SetThreadpoolTimer` with `NULL` for `pftDueTime` and 0 for `msPeriod` and `msWindowLength` to cancel ongoing timers. **]**

**SRS_THREADPOOL_WIN32_42_026: [** `threadpool_timer_cancel` shall call `WaitForThreadpoolTimerCallbacks`. **]**

**SRS_THREADPOOL_WIN32_07_006: [** `threadpool_timer_cancel` shall release the exclusive lock. **]**

### threadpool_timer_dispose

```c
static void threadpool_timer_dispose(THREADPOOL_TIMER* timer);
```

`threadpool_timer_dispose` stops the timer when thre reference count of the THANDLE(THREADPOOL_TIMER) created by `threadpool_timer_start` reaches 0 and cleans up its resources.

**SRS_THREADPOOL_WIN32_42_012: [** `threadpool_timer_dispose` shall call `SetThreadpoolTimer` with `NULL` for `pftDueTime` and 0 for `msPeriod` and `msWindowLength` to cancel ongoing timers. **]**

**SRS_THREADPOOL_WIN32_42_013: [** `threadpool_timer_dispose` shall call `WaitForThreadpoolTimerCallbacks`. **]**

**SRS_THREADPOOL_WIN32_42_014: [** `threadpool_timer_dispose` shall call `CloseThreadpoolTimer`. **]**

**SRS_THREADPOOL_WIN32_07_001: [** `threadpool_timer_dispose` shall call `srw_lock_ll_deinit`. **]**

### on_timer_callback

```c
static VOID CALLBACK on_timer_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_TIMER timer);
```

`on_work_callback` executes the work function passed to `threadpool_schedule_work`.

**SRS_THREADPOOL_WIN32_42_016: [** If `context` is `NULL`, `on_work_callback` shall return. **]**

**SRS_THREADPOOL_WIN32_42_017: [** Otherwise `context` shall be used as the context created in `threadpool_schedule_work`. **]**

**SRS_THREADPOOL_WIN32_42_018: [** The `work_function` callback passed to `threadpool_schedule_work` shall be called, passing to it the `work_function_context` argument passed to `threadpool_schedule_work`. **]**

### threadpool_create_work_item

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_WORK_ITEM), threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_create_work_item` creates a work item to be executed by the threadpool.

**SRS_THREADPOOL_WIN32_05_004: [** If `threadpool` is `NULL`, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**SRS_THREADPOOL_WIN32_05_005: [** If `work_function` is `NULL`, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**SRS_THREADPOOL_WIN32_05_006: [** Otherwise `threadpool_create_work_item` shall allocate a context of type `THANDLE(THREADPOOL_WORK_ITEM)` with the dispose function being `threadpool_dispose_work_item`. **]**

**SRS_THREADPOOL_WIN32_05_007: [** If any error occurs, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**SRS_THREADPOOL_WIN32_05_008: [** `threadpool_create_work_item` shall create `threadpool_work_item` member variable `ptp_work` of type `PTP_WORK` by calling `CreateThreadpoolWork` to set the callback function as `on_work_callback_v2`. **]**

**SRS_THREADPOOL_WIN32_05_009: [** If there are no errors then this `threadpool_work_item` of type `THANDLE(THREADPOOL_WORK_ITEM)` would be returned indicating a succcess to the caller. **]**

**SRS_THREADPOOL_WIN32_05_010: [** If any error occurs, `threadpool_create_work_item` shall fail, free the newly created context and return a `NULL` value. **]**


### threadpool_schedule_work_item

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THANDLE(THREADPOOL_WORK_ITEM), threadpool_work_item);
```

`threadpool_schedule_work_item` schedules a work item to be executed by the threadpool.

**SRS_THREADPOOL_WIN32_05_011: [** If `threadpool` is NULL, `threadpool_schedule_work_item` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_05_012: [** If `threadpool_work_item` is NULL, `threadpool_schedule_work_item` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_WIN32_05_013: [** `threadpool_schedule_work_item` shall call `SubmitThreadpoolWork` to submit the work item for execution. **]**

### threadpool_dispose_work_item

```c
static void threadpool_dispose_work_item(THANDLE(THREADPOOL_WORK_ITEM) threadpool_work_item);
```

`threadpool_dispose_work_item` closes the `ptp_work` member variable in `threadpool_work_item`.

**SRS_THREADPOOL_WIN32_05_016: [** `threadpool_dispose_work_item` shall call `WaitForThreadpoolWorkCallbacks` to wait on all outstanding tasks being scheduled on this `ptp_work`. **]**

**SRS_THREADPOOL_WIN32_05_017: [** `threadpool_dispose_work_item` shall call `CloseThreadpoolWork` to close `ptp_work`. **]**