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
typedef struct TIMER_INSTANCE_TAG* TIMER_INSTANCE_HANDLE;
typedef struct THREADPOOL_WORK_ITEM_TAG* THREADPOOL_WORK_ITEM_HANDLE;
typedef void (*THREADPOOL_WORK_FUNCTION)(void* context);

THANDLE_TYPE_DECLARE(THREADPOOL);

MOCKABLE_FUNCTION(, THANDLE(THREADPOOL), threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, int, threadpool_open, THANDLE(THREADPOOL), threadpool);
MOCKABLE_FUNCTION(, void, threadpool_close, THANDLE(THREADPOOL), threadpool);

MOCKABLE_FUNCTION(, THREADPOOL_WORK_ITEM_HANDLE, threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_ITEM_HANDLE, work_item_context);

MOCKABLE_FUNCTION(, void, threadpool_work_context_destroy, THREADPOOL_WORK_ITEM_HANDLE, work_item_context);

MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);

MOCKABLE_FUNCTION(, int, threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);

MOCKABLE_FUNCTION(, int, threadpool_timer_restart, TIMER_INSTANCE_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);

MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, TIMER_INSTANCE_HANDLE, timer);

MOCKABLE_FUNCTION(, void, threadpool_timer_destroy, TIMER_INSTANCE_HANDLE, timer);
```

### threadpool_create

```c
MOCKABLE_FUNCTION(, THREADPOOL_HANDLE, threadpool_create, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`threadpool_create` creates a threadpool object that can execute work items.

**NON_THREADPOOL_01_001: [** `threadpool_create` shall allocate a new threadpool object and on success shall return a non-`NULL` handle. **]**

**NON_THREADPOOL_01_002: [** If `execution_engine` is `NULL`, `threadpool_create` shall fail and return `NULL`. **]**

**NON_THREADPOOL_01_003: [** If any error occurs, `threadpool_create` shall fail and return `NULL`. **]**

### threadpool_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_destroy, THREADPOOL_HANDLE, threadpool);
```

`threadpool_destroy` frees all resources associated with `threadpool`.

**NON_THREADPOOL_01_004: [** If `threadpool` is `NULL`, `threadpool_destroy` shall return. **]**

**NON_THREADPOOL_01_005: [** Otherwise, `threadpool_destroy` shall free all resources associated with `threadpool`. **]**

**NON_THREADPOOL_01_006: [** While `threadpool` is OPENING, `threadpool_destroy` shall wait for the open to complete either successfully or with error. **]**

**NON_THREADPOOL_01_007: [** `threadpool_destroy` shall perform an implicit close if `threadpool` is OPEN. **]**

### threadpool_open

```c
MOCKABLE_FUNCTION(, int, threadpool_open, THREADPOOL_HANDLE, threadpool);
```

`threadpool_open` opens the threadpool object so that subsequent calls to `threadpool_schedule_work` can be made.

**NON_THREADPOOL_01_008: [** If `threadpool` is `NULL`, `threadpool_open` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_01_011: [** `threadpool_open` shall switch the state to OPENING and perform any actions needed to open the `threadpool` object. **]**

**NON_THREADPOOL_01_012: [** On success, `threadpool_open` shall return 0. **]**

**NON_THREADPOOL_01_013: [** If `threadpool` is already OPEN or OPENING, `threadpool_open` shall fail and return a non-zero value. **]**

### threadpool_close

```c
MOCKABLE_FUNCTION(, void, threadpool_close, THREADPOOL_HANDLE, threadpool);
```

`threadpool_close` closes an open `threadpool`.

**NON_THREADPOOL_01_016: [** If `threadpool` is `NULL`, `threadpool_close` shall return. **]**

**NON_THREADPOOL_01_017: [** Otherwise, `threadpool_close` shall switch the state to CLOSING. **]**

**NON_THREADPOOL_01_018: [** Then `threadpool_close` shall close the threadpool, leaving it in a state where an `threadpool_open` can be performed. **]**

**NON_THREADPOOL_01_019: [** If `threadpool` is not OPEN, `threadpool_close` shall return. **]**

### threadpool_create_work_item

```c
MOCKABLE_FUNCTION(, THREADPOOL_WORK_ITEM_HANDLE, threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_create_work_item` creates a work item to be executed by the threadpool. This function separates the creation of the work item from scheduling it so that threadpool_schedule_work_item can be called later and cannot fail.

**NON_THREADPOOL_05_001: [** If `threadpool` is `NULL`, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**NON_THREADPOOL_05_002: [** If `work_function` is `NULL`, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**NON_THREADPOOL_05_003: [** Otherwise `threadpool_create_work_item` shall allocate a context of type `THREADPOOL_WORK_ITEM_HANDLE` to save threadpool related variables. **]**

**NON_THREADPOOL_05_004: [** If any error occurs, `threadpool_create_work_item` shall fail and return a `NULL` value. **]**

**NON_THREADPOOL_05_005: [** `threadpool_create_work_item` shall create an individual work item and if needed set a local callback function. **]**

**NON_THREADPOOL_05_006: [** If any error occurs, `threadpool_create_work_item` shall fail, free the newly created context and return a `NULL` value. **]**

### threadpool_schedule_work_item

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_ITEM_HANDLE, work_item_context);
```

`threadpool_schedule_work_item` schedules a work item to be executed by the threadpool. This API is called multiple times on the same `THREADPOOL_WORK_ITEM_HANDLE` and fails only if the arguments are passed `NULL`.

**NON_THREADPOOL_05_007: [** If `threadpool` is NULL, `threadpool_schedule_work_item` shall fail and return a `non-zero` value. **]**

**NON_THREADPOOL_05_008: [** If `work_function` is NULL, `threadpool_schedule_work_item` shall fail and return a `non-zero` value. **]**

**NON_THREADPOOL_05_009: [** `threadpool_schedule_work_item` shall submit the threadpool work item for execution. **]**

### threadpool_work_context_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_destroy_work_item, THREADPOOL_WORK_ITEM_HANDLE, work_item_context);
```

`threadpool_destroy_work_item` closes and frees all the member variables in the context of type `THREADPOOL_WORK_ITEM_HANDLE` and then frees the context

**NON_THREADPOOL_05_010: [** If `work_item_context` is `NULL`, `threadpool_destroy_work_item` shall fail and not do anything before returning. **]**

**NON_THREADPOOL_05_011: [** `threadpool_destroy_work_item` shall close and free all the members of `THREADPOOL_WORK_ITEM_HANDLE`. **]**

**NON_THREADPOOL_05_012: [** `threadpool_destroy_work_item` shall free the `work_item_context` of type `THREADPOOL_WORK_ITEM_HANDLE`. **]**

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
MOCKABLE_FUNCTION(, int, threadpool_timer_start, THREADPOOL_HANDLE, threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context, TIMER_INSTANCE_HANDLE*, timer_handle);
```

`threadpool_timer_start` starts a threadpool timer which runs after `start_delay_ms` milliseconds and then runs again every `timer_period_ms` milliseconds until `threadpool_timer_destroy` is called. The `timer_handle` must be stopped before closing/destroying the threadpool.

**NON_THREADPOOL_42_001: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_42_002: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_42_003: [** If `timer_handle` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_42_004: [** `work_function_context` shall be allowed to be `NULL`. **]**

**NON_THREADPOOL_42_005: [** `threadpool_timer_start` shall allocate a context for the timer being started and store `work_function` and `work_function_context` in it. **]**

**NON_THREADPOOL_42_007: [** If `timer_period_ms` is 0 then `threadpool_timer_start` shall queue for execution the function `work_function` to be executed once after `start_delay_ms` pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_008: [** Otherwise `threadpool_timer_start` shall queue for execution the function `work_function` to be executed after `start_delay_ms` and every `timer_period_ms` thereafter and pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_009: [** If any error occurs, `threadpool_timer_start` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_42_010: [** `threadpool_timer_start` shall return the allocated handle in `timer_handle`. **]**

**NON_THREADPOOL_42_011: [** `threadpool_timer_start` shall succeed and return 0. **]**

### threadpool_timer_restart

```c
MOCKABLE_FUNCTION(, int, threadpool_timer_restart, TIMER_INSTANCE_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`threadpool_timer_restart` changes the delay and period of an existing timer.

**NON_THREADPOOL_42_015: [** If `timer` is `NULL`, `threadpool_timer_restart` shall fail and return a non-zero value. **]**

**NON_THREADPOOL_42_016: [** `threadpool_timer_restart` shall stop execution of the existing timer and wait for any current executions to complete. **]**

**NON_THREADPOOL_42_017: [** If `timer_period_ms` is 0 then `threadpool_timer_restart` shall queue for execution the function `work_function` to be executed once after `start_delay_ms` pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_018: [** Otherwise `threadpool_timer_restart` shall queue for execution the function `work_function` to be executed after `start_delay_ms` and every `timer_period_ms` thereafter and pass `context` to it when it executes. **]**

**NON_THREADPOOL_42_019: [** `threadpool_timer_restart` shall succeed and return 0. **]**

### threadpool_timer_cancel

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, TIMER_INSTANCE_HANDLE, timer);
```

`threadpool_timer_cancel` stops the timer and waits for any pending callbacks. Afterward, the timer may be resumed with a new time by calling `threadpool_timer_restart` or cleaned up by calling `threadpool_timer_destroy`.

**NON_THREADPOOL_42_020: [** If `timer` is `NULL`, `threadpool_timer_cancel` shall fail and return. **]**

**NON_THREADPOOL_42_021: [** `threadpool_timer_cancel` shall stop further execution of the timer and wait for any current executions to complete. **]**

### threadpool_timer_destroy

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_destroy, TIMER_INSTANCE_HANDLE, timer);
```

`threadpool_timer_destroy` stops the timer started by `threadpool_timer_start` and cleans up its resources.

**NON_THREADPOOL_42_012: [** If `timer` is `NULL`, `threadpool_timer_destroy` shall fail and return. **]**

**NON_THREADPOOL_42_013: [** `threadpool_timer_destroy` shall stop further execution of the timer and wait for any current executions to complete. **]**

**NON_THREADPOOL_42_014: [** `threadpool_timer_destroy` shall free all resources in `timer`. **]**
