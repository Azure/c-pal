# `threadpool` interface requirements

## Overview

`threadpool` is an interface for a threadpool that allows scheduling work items.

A `threadpool` object receives an execution engine at creation time in order to be able to schedule all asynchronous work using the execution engine.

The `threadpool` interface supports:
 - Scheduling a single work item (`threadpool_schedule_work`)
 - Scheduling a timer function to execute at a regular interval
   - `threadpool_timer_start`
   - `threadpool_timer_destroy`
   - `threadpool_timer_restart`
   - `threadpool_timer_cancel`

The lifetime of the execution engine should supersede the lifetime of the `threadpool` object.

## Exposed API

```c
typedef struct THREADPOOL_TAG THREADPOOL;
typedef struct THREADPOOL_TIMER_TAG* THREADPOOL_TIMER *;
typedef struct THREADPOOL_WORK_ITEM_TAG THREADPOOL_WORK_ITEM;
typedef struct THREADPOOL_WORK_ITEM_TAG* THREADPOOL_WORK_ITEM_HANDLE;
typedef void (*THREADPOOL_WORK_FUNCTION)(void* context);

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
MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`threadpool_create` creates a threadpool object that can execute work items.

**NON_THREADPOOL_01_001: [** `threadpool_create` shall allocate a new `threadpool` object and on success shall return a non-`NULL` handle. **]**

**NON_THREADPOOL_01_002: [** If `execution_engine` is `NULL`, `threadpool_create` shall fail and return `NULL`. **]**

**NON_THREADPOOL_01_011: [** `threadpool_create` shall perform any actions needed to open the `threadpool` object. **]**

**NON_THREADPOOL_01_003: [** If any error occurs, `threadpool_create` shall fail and return `NULL`. **]**

### threadpool_create_work_item

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_WORK_ITEM), threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_create_work_item` creates a work item to be executed by the threadpool. This function separates the creation of the work item from scheduling it so that `threadpool_schedule_work_item` can be called later and cannot fail.

**NON_THREADPOOL_05_001: [** If `threadpool` is `NULL`, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**NON_THREADPOOL_05_002: [** If `work_function` is `NULL`, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**NON_THREADPOOL_05_003: [** Otherwise `threadpool_create_work_item` shall allocate a context of type `THANDLE(THREADPOOL_WORK_ITEM)` to save threadpool related variables. **]**

**NON_THREADPOOL_05_004: [** If any error occurs, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**NON_THREADPOOL_05_006: [** If there are no errors then the work item of type `THANDLE(THREADPOOL_WORK_ITEM)` is created and returned to the caller. **]**

**NON_THREADPOOL_05_007: [** If any error occurs, `threadpool_create_work_item` shall fail, free the newly created context and return a `NULL` value. **]**

### threadpool_schedule_work_item

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THANDLE(THREADPOOL_WORK_ITEM), threadpool_work_item);
```

`threadpool_schedule_work_item` schedules a work item to be executed by the threadpool. This API can be called multiple times on the same `threadpool_work_item` and fails only if the arguments are passed `NULL`.

**NON_THREADPOOL_05_008: [** If `threadpool` is NULL, `threadpool_schedule_work_item` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_05_009: [** If `threadpool_work_item` is NULL, `threadpool_schedule_work_item` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_05_010: [** `threadpool_schedule_work_item` shall submit the `threadpool_work_item` for execution. **]**

### threadpool_schedule_work

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THREADPOOL_HANDLE, threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_schedule_work` schedules a work item to be executed by the threadpool.

**NON_THREADPOOL_01_020: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_01_021: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_01_022: [** `work_function_context` shall be allowed to be `NULL`. **]**

**NON_THREADPOOL_01_023: [** Otherwise `threadpool_schedule_work` shall queue for execution the function `work_function` and pass `context` to it when it executes. **]**

Note: There are no guarantees regarding the order of execution for the `work_function` callbacks.

**NON_THREADPOOL_01_024: [** If any error occurs, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

### threadpool_timer_start

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_TIMER), threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_timer_start` starts a threadpool timer which runs after `start_delay_ms` milliseconds and then runs again every `timer_period_ms` milliseconds until `threadpool_timer_destroy` is called. The `timer_handle` must be stopped before closing/destroying the threadpool.

**NON_THREADPOOL_42_001: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return NULL. **]**

**NON_THREADPOOL_42_002: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return NULL. **]**

**NON_THREADPOOL_42_004: [** `work_function_context` shall be allowed to be `NULL`. **]**

**NON_THREADPOOL_42_005: [** `threadpool_timer_start` shall allocate memory for `THANDLE(THREADPOOL_TIMER)`, passing `threadpool_timer_dispose` as dispose function and store `work_function` and `work_function_ctx` in it. **]**

**NON_THREADPOOL_07_001: [** `threadpool_timer_start` shall initialize the lock guarding the timer state by calling `srw_lock_ll_init`. **]**

**NON_THREADPOOL_42_007: [** If `timer_period_ms` is 0 then `threadpool_timer_start` shall queue for execution the function `work_function` to be executed once after `start_delay_ms` pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_008: [** Otherwise `threadpool_timer_start` shall queue for execution the function `work_function` to be executed after `start_delay_ms` and every `timer_period_ms` thereafter and pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_009: [** If any error occurs, `threadpool_timer_start` shall fail and return NULL. **]**

**NON_THREADPOOL_42_010: [** `threadpool_timer_start` shall return the allocated handle in `timer_handle`. **]**

**NON_THREADPOOL_42_011: [** `threadpool_timer_start` shall succeed and return a non-NULL handle. **]**

### threadpool_timer_restart

```c
MOCKABLE_FUNCTION(, int, threadpool_timer_restart, THANDLE(THREADPOOL_TIMER), timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`threadpool_timer_restart` changes the delay and period of an existing timer.

**NON_THREADPOOL_42_015: [** If `timer` is `NULL`, `threadpool_timer_restart` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_07_002: [** `threadpool_timer_restart` shall acquire an exclusive lock. **]**

**NON_THREADPOOL_42_016: [** `threadpool_timer_restart` shall stop execution of the existing timer and wait for any current executions to complete. **]**

**NON_THREADPOOL_42_017: [** If `timer_period_ms` is 0 then `threadpool_timer_restart` shall queue for execution the function `work_function` to be executed once after `start_delay_ms` pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_018: [** Otherwise `threadpool_timer_restart` shall queue for execution the function `work_function` to be executed after `start_delay_ms` and every `timer_period_ms` thereafter and pass `context` to it when it executes. **]**

**NON_THREADPOOL_07_003: [** `threadpool_timer_restart` shall release the exclusive lock. **]**

**NON_THREADPOOL_42_019: [** `threadpool_timer_restart` shall succeed and return 0. **]**

### threadpool_timer_cancel

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, THREADPOOL_TIMER *, timer);
```

`threadpool_timer_cancel` stops the timer and waits for any pending callbacks. Afterward, the timer may be resumed with a new time by calling `threadpool_timer_restart` or cleaned up by calling `threadpool_timer_destroy`.

**NON_THREADPOOL_42_020: [** If `timer` is `NULL`, `threadpool_timer_cancel` shall fail and return. **]**

**NON_THREADPOOL_07_004: [** `threadpool_timer_cancel` shall acquire an exclusive lock. **]**

**NON_THREADPOOL_42_021: [** `threadpool_timer_cancel` shall stop further execution of the timer and wait for any current executions to complete. **]**

**NON_THREADPOOL_07_005: [** `threadpool_timer_cancel` shall release the exclusive lock. **]**

### threadpool_timer_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_destroy, THREADPOOL_TIMER *, timer);
```

`threadpool_timer_destroy` stops the timer started by `threadpool_timer_start` and cleans up its resources.

**NON_THREADPOOL_42_012: [** If `timer` is `NULL`, `threadpool_timer_destroy` shall fail and return. **]**

**NON_THREADPOOL_42_013: [** `threadpool_timer_destroy` shall stop further execution of the timer and wait for any current executions to complete. **]**

**NON_THREADPOOL_42_014: [** `threadpool_timer_destroy` shall free all resources in `timer`. **]**

**NON_THREADPOOL_07_006: [** `threadpool_timer_dispose` shall call `srw_lock_ll_deinit`. **]**
