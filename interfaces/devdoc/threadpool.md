`threadpool` interface requirements
================

## Overview

`threadpool` is an interface for a threadpool that allows scheduling work items.

A `threadpool` object receives an execution engine at creation time in order to be able to schedule all asynchronous work using the execution engine.

The `threadpool` interface supports:
 - Scheduling a single work item (`threadpool_schedule_work`)
 - Scheduling a timer function to execute at a regular interval (`threadpool_start_timer`/`threadpool_stop_timer`)

The lifetime of the execution engine should supersede the lifetime of the `threadpool` object.

## Exposed API

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

MOCKABLE_FUNCTION(, int, threadpool_start_timer, THREADPOOL_HANDLE, threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);

MOCKABLE_FUNCTION(, void, threadpool_stop_timer, TIMER_INSTANCE_HANDLE, timer);
```

### threadpool_create

```c
MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`threadpool_create` creates a threadpool object that can execute work items.

**SRS_THREADPOOL_01_001: [** `threadpool_create` shall allocate a new threadpool object and on success shall return a non-`NULL` handle. **]**

**SRS_THREADPOOL_01_002: [** If `execution_engine` is `NULL`, `threadpool_create` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_01_003: [** If any error occurs, `threadpool_create` shall fail and return `NULL`. **]**

### threadpool_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_destroy, THREADPOOL_HANDLE, threadpool);
```

`threadpool_destroy` frees all resources associated with `threadpool`.

**SRS_THREADPOOL_01_004: [** If `threadpool` is `NULL`, `threadpool_destroy` shall return. **]**

**SRS_THREADPOOL_01_005: [** Otherwise, `threadpool_destroy` shall free all resources associated with `threadpool`. **]**

**SRS_THREADPOOL_01_006: [** While `threadpool` is OPENING, `threadpool_destroy` shall wait for the open to complete either successfully or with error. **]**

**SRS_THREADPOOL_01_007: [** `threadpool_destroy` shall perform an implicit close if `threadpool` is OPEN. **]**

### threadpool_open_async

```c
MOCKABLE_FUNCTION(, int, threadpool_open_async, THREADPOOL_HANDLE, threadpool, ON_THREADPOOL_OPEN_COMPLETE, on_open_complete, void*, on_open_complete_context);
```

`threadpool_open_async` opens the threadpool object so that subsequent calls to `threadpool_schedule_work` can be made.

**SRS_THREADPOOL_01_008: [** If `threadpool` is `NULL`, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_01_009: [** If `on_open_complete` is `NULL`, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_01_010: [** `on_open_complete_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_01_011: [** Otherwise, `threadpool_open_async` shall switch the state to OPENING and perform any actions needed to open the `threadpool` object. **]**

**SRS_THREADPOOL_01_012: [** On success, `threadpool_open_async` shall return 0. **]**

**SRS_THREADPOOL_01_013: [** If `threadpool` is already OPEN or OPENING, `threadpool_open_async` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_01_014: [** When opening the threadpool object completes successfully, `on_open_complete_context` shall be called with `THREADPOOL_OPEN_OK`. **]**

**SRS_THREADPOOL_01_015: [** When opening the threadpool object completes with failure, `on_open_complete_context` shall be called with `THREADPOOL_OPEN_ERROR`. **]**

### threadpool_close

```c
MOCKABLE_FUNCTION(, void, threadpool_close, THREADPOOL_HANDLE, threadpool);
```

`threadpool_close` closes an open `threadpool`.

**SRS_THREADPOOL_01_016: [** If `threadpool` is `NULL`, `threadpool_close` shall return. **]**

**SRS_THREADPOOL_01_017: [** Otherwise, `threadpool_close` shall switch the state to CLOSING. **]**

**SRS_THREADPOOL_01_018: [** Then `threadpool_close` shall close the threadpool, leaving it in a state where an `threadpool_open_async` can be performed. **]**

**SRS_THREADPOOL_01_019: [** If `threadpool` is not OPEN, `threadpool_close` shall return. **]**

### threadpool_schedule_work

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THREADPOOL_HANDLE, threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_schedule_work` schedules a work item to be executed by the threadpool.

**SRS_THREADPOOL_01_020: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_01_021: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_01_022: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_01_023: [** Otherwise `threadpool_schedule_work` shall queue for execution the function `work_function` and pass `context` to it when it executes. **]**

Note: There are no guarantees regarding the order of execution for the `work_function` callbacks.

**SRS_THREADPOOL_01_024: [** If any error occurs, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

### threadpool_start_timer

```c
MOCKABLE_FUNCTION(, int, threadpool_start_timer, THREADPOOL_HANDLE, threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);
```

`threadpool_start_timer` starts a threadpool timer which runs after `start_delay_ms` milliseconds and then runs again every `timer_period_ms` milliseconds until `threadpool_stop_timer` is called. The `timer_handle` must be stopped before closing/destroying the threadpool.

**SRS_THREADPOOL_42_001: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_42_002: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_42_003: [** If `timer_handle` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_42_004: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_42_005: [** `threadpool_start_timer` shall allocate a context for the timer being started and store `work_function` and `work_function_context` in it. **]**

**SRS_THREADPOOL_42_007: [** If `timer_period_ms` is 0 then `threadpool_start_timer` shall queue for execution the function `work_function` to be executed once after `start_delay_ms` pass `context` to it when it executes. **]**

**SRS_THREADPOOL_42_008: [** Otherwise `threadpool_start_timer` shall queue for execution the function `work_function` to be executed after `start_delay_ms` and every `timer_period_ms` thereafter and pass `context` to it when it executes. **]**

**SRS_THREADPOOL_42_009: [** If any error occurs, `threadpool_start_timer` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_42_010: [** `threadpool_start_timer` shall return the allocated handle in `timer_handle`. **]**

**SRS_THREADPOOL_42_011: [** `threadpool_start_timer` shall succeed and return 0. **]**

### threadpool_stop_timer

```c
MOCKABLE_FUNCTION(, void, threadpool_stop_timer, TIMER_INSTANCE_HANDLE, timer);
```

`threadpool_stop_timer` stops the timer started by `threadpool_start_timer` and cleans up its resources.

**SRS_THREADPOOL_42_012: [** If `timer` is `NULL`, `threadpool_stop_timer` shall fail and return. **]**

**SRS_THREADPOOL_42_013: [** `threadpool_stop_timer` shall stop further execution of the timer and wait for any current executions to complete. **]**

**SRS_THREADPOOL_42_014: [** `threadpool_stop_timer` shall free all resources in `timer`. **]**
