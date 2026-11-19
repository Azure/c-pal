# threadpool_linux

## Overview

`threadpool_linux` is the Linux implementation of the `threadpool` interface.

## Design

`threadpool_linux` maps the `threadpool` API onto the work objects and timers of `execution_engine_linux`, the same way `threadpool_win32` maps it onto the Win32 thread pool:

| `threadpool` | `execution_engine_linux` | Win32 equivalent |
|---|---|---|
| `threadpool_create` | holds a reference to the execution engine | cleanup group |
| `threadpool_create_work_item` | `execution_engine_linux_create_work` | `CreateThreadpoolWork` |
| `threadpool_schedule_work_item` | `execution_engine_linux_submit_work` | `SubmitThreadpoolWork` |
| work item dispose | `execution_engine_linux_wait_for_work` + `execution_engine_linux_close_work` | `WaitForThreadpoolWorkCallbacks` + `CloseThreadpoolWork` |
| `threadpool_schedule_work` | one-time work object | `CreateThreadpoolWork` + `SubmitThreadpoolWork` |
| `threadpool_timer_start` | `execution_engine_linux_create_timer` + `execution_engine_linux_set_timer` | `CreateThreadpoolTimer` + `SetThreadpoolTimer` |
| `threadpool_timer_restart` | `execution_engine_linux_set_timer` | `SetThreadpoolTimer` |
| `threadpool_timer_cancel` | `execution_engine_linux_cancel_timer` + `execution_engine_linux_wait_for_work` (dropping pending invocations) | `SetThreadpoolTimer(NULL)` + `WaitForThreadpoolTimerCallbacks(TRUE)` |

All callbacks run on the worker threads of the execution engine, which are shared by every `threadpool` created on it.

`threadpool_schedule_work_item`, `threadpool_timer_restart` and `threadpool_timer_cancel` do not allocate and only fail for invalid arguments.

Releasing the last reference to a work item waits for its pending and running callbacks. Releasing the last reference to a timer cancels it and waits for its running callback. Releasing the last reference to a `threadpool` waits for the callbacks scheduled with `threadpool_schedule_work`. As with Win32, work items and timers must be released before the `threadpool` they were created on, and none of these may be released from their own callbacks.

## Exposed API

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

`threadpool_create` creates a threadpool object that schedules callbacks on the worker threads of `execution_engine`.

**SRS_THREADPOOL_LINUX_42_001: [** If `execution_engine` is `NULL`, `threadpool_create` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_002: [** `threadpool_create` shall allocate a new threadpool object with `threadpool_dispose` as dispose function and on success shall return a non-`NULL` handle. **]**

**SRS_THREADPOOL_LINUX_42_003: [** `threadpool_create` shall increment the reference count on `execution_engine` and store it. **]**

**SRS_THREADPOOL_LINUX_42_004: [** `threadpool_create` shall set the count of pending scheduled work to 0. **]**

**SRS_THREADPOOL_LINUX_42_005: [** If any error occurs, `threadpool_create` shall fail and return `NULL`. **]**

### threadpool_dispose

```c
static void threadpool_dispose(THREADPOOL* threadpool);
```

`threadpool_dispose` is called when the last reference to the threadpool is released.

**SRS_THREADPOOL_LINUX_42_006: [** `threadpool_dispose` shall wait for the count of pending scheduled work to reach 0 by calling `InterlockedHL_WaitForValue`. **]**

**SRS_THREADPOOL_LINUX_42_007: [** `threadpool_dispose` shall decrement the reference count on the execution engine. **]**

### threadpool_create_work_item

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_WORK_ITEM), threadpool_create_work_item, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_create_work_item` creates a work item that can be scheduled many times with `threadpool_schedule_work_item`.

**SRS_THREADPOOL_LINUX_42_008: [** If `threadpool` is `NULL`, `threadpool_create_work_item` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_009: [** If `work_function` is `NULL`, `threadpool_create_work_item` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_010: [** `threadpool_create_work_item` shall allocate a `THANDLE(THREADPOOL_WORK_ITEM)` with `threadpool_dispose_work_item` as dispose function and store `work_function` and `work_function_context` in it. **]**

**SRS_THREADPOOL_LINUX_42_011: [** `threadpool_create_work_item` shall create a work object by calling `execution_engine_linux_create_work` with `on_work_item_callback` and the work item as context. **]**

**SRS_THREADPOOL_LINUX_42_012: [** `threadpool_create_work_item` shall succeed and return the work item. **]**

**SRS_THREADPOOL_LINUX_42_013: [** If any error occurs, `threadpool_create_work_item` shall fail, free the work item and return `NULL`. **]**

### on_work_item_callback

```c
static void on_work_item_callback(void* context);
```

**SRS_THREADPOOL_LINUX_42_014: [** `on_work_item_callback` shall call the `work_function` of the work item with its `work_function_context`. **]**

### threadpool_dispose_work_item

```c
static void threadpool_dispose_work_item(THREADPOOL_WORK_ITEM* threadpool_work_item);
```

**SRS_THREADPOOL_LINUX_42_015: [** `threadpool_dispose_work_item` shall wait for the pending and running callbacks of the work item by calling `execution_engine_linux_wait_for_work` with `cancel_pending` set to `false`. **]**

**SRS_THREADPOOL_LINUX_42_016: [** `threadpool_dispose_work_item` shall close the work object by calling `execution_engine_linux_close_work`. **]**

### threadpool_schedule_work_item

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work_item, THANDLE(THREADPOOL), threadpool, THANDLE(THREADPOOL_WORK_ITEM), threadpool_work_item);
```

`threadpool_schedule_work_item` schedules one callback of a work item. It can be called any number of times on the same work item, each call results in one callback, and it fails only for invalid arguments.

**SRS_THREADPOOL_LINUX_42_017: [** If `threadpool` is `NULL`, `threadpool_schedule_work_item` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_LINUX_42_018: [** If `threadpool_work_item` is `NULL`, `threadpool_schedule_work_item` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_LINUX_42_019: [** `threadpool_schedule_work_item` shall submit the work object of `threadpool_work_item` by calling `execution_engine_linux_submit_work`. **]**

**SRS_THREADPOOL_LINUX_42_020: [** `threadpool_schedule_work_item` shall succeed and return 0. **]**

### threadpool_schedule_work

```c
MOCKABLE_FUNCTION(, int, threadpool_schedule_work, THANDLE(THREADPOOL), threadpool, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_schedule_work` schedules one callback of `work_function`.

**SRS_THREADPOOL_LINUX_42_021: [** If `threadpool` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_LINUX_42_022: [** If `work_function` is `NULL`, `threadpool_schedule_work` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_LINUX_42_023: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_024: [** `threadpool_schedule_work` shall allocate a context that stores the threadpool, `work_function` and `work_function_context`. **]**

**SRS_THREADPOOL_LINUX_42_025: [** `threadpool_schedule_work` shall create a work object by calling `execution_engine_linux_create_work` with `on_scheduled_work_callback` and the context. **]**

**SRS_THREADPOOL_LINUX_42_026: [** `threadpool_schedule_work` shall increment the count of pending scheduled work by calling `interlocked_increment`. **]**

**SRS_THREADPOOL_LINUX_42_027: [** `threadpool_schedule_work` shall submit the work object by calling `execution_engine_linux_submit_work` and return 0. **]**

**SRS_THREADPOOL_LINUX_42_028: [** If any error occurs, `threadpool_schedule_work` shall free the context, fail and return a non-zero value. **]**

### on_scheduled_work_callback

```c
static void on_scheduled_work_callback(void* context);
```

**SRS_THREADPOOL_LINUX_42_029: [** `on_scheduled_work_callback` shall call `work_function` with `work_function_context`. **]**

**SRS_THREADPOOL_LINUX_42_030: [** `on_scheduled_work_callback` shall close the work object by calling `execution_engine_linux_close_work`. **]**

**SRS_THREADPOOL_LINUX_42_031: [** `on_scheduled_work_callback` shall free the context. **]**

**SRS_THREADPOOL_LINUX_42_032: [** `on_scheduled_work_callback` shall decrement the count of pending scheduled work of the threadpool by calling `interlocked_decrement` and, if it reached 0, call `wake_by_address_all`. **]**

### threadpool_timer_start

```c
MOCKABLE_FUNCTION(, THANDLE(THREADPOOL_TIMER), threadpool_timer_start, THANDLE(THREADPOOL), threadpool, uint32_t, start_delay_ms, uint32_t, timer_period_ms, THREADPOOL_WORK_FUNCTION, work_function, void*, work_function_context);
```

`threadpool_timer_start` starts a timer that calls `work_function` after `start_delay_ms` milliseconds and then every `timer_period_ms` milliseconds, or only once if `timer_period_ms` is 0.

**SRS_THREADPOOL_LINUX_42_033: [** If `threadpool` is `NULL`, `threadpool_timer_start` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_034: [** If `work_function` is `NULL`, `threadpool_timer_start` shall fail and return `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_035: [** `work_function_context` shall be allowed to be `NULL`. **]**

**SRS_THREADPOOL_LINUX_42_036: [** `threadpool_timer_start` shall allocate a `THANDLE(THREADPOOL_TIMER)` with `threadpool_timer_dispose` as dispose function and store `work_function` and `work_function_context` in it. **]**

**SRS_THREADPOOL_LINUX_42_037: [** `threadpool_timer_start` shall initialize the lock guarding the timer state by calling `srw_lock_ll_init`. **]**

**SRS_THREADPOOL_LINUX_42_038: [** `threadpool_timer_start` shall create the timer by calling `execution_engine_linux_create_timer` with `on_timer_callback` and the timer as context. **]**

**SRS_THREADPOOL_LINUX_42_039: [** `threadpool_timer_start` shall arm the timer by calling `execution_engine_linux_set_timer` with `start_delay_ms` and `timer_period_ms`. **]**

**SRS_THREADPOOL_LINUX_42_040: [** `threadpool_timer_start` shall succeed and return the timer. **]**

**SRS_THREADPOOL_LINUX_42_041: [** If any error occurs, `threadpool_timer_start` shall free the resources it allocated, fail and return `NULL`. **]**

### on_timer_callback

```c
static void on_timer_callback(void* context);
```

**SRS_THREADPOOL_LINUX_42_042: [** `on_timer_callback` shall call the `work_function` of the timer with its `work_function_context`. **]**

### threadpool_timer_restart

```c
MOCKABLE_FUNCTION(, int, threadpool_timer_restart, THANDLE(THREADPOOL_TIMER), timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`threadpool_timer_restart` changes the delay and period of an existing timer. It does not wait for a running callback, so it can be called from the timer's own callback.

**SRS_THREADPOOL_LINUX_42_043: [** If `timer` is `NULL`, `threadpool_timer_restart` shall fail and return a non-zero value. **]**

**SRS_THREADPOOL_LINUX_42_044: [** `threadpool_timer_restart` shall acquire the timer lock exclusively by calling `srw_lock_ll_acquire_exclusive`. **]**

**SRS_THREADPOOL_LINUX_42_045: [** `threadpool_timer_restart` shall re-arm the timer by calling `execution_engine_linux_set_timer` with `start_delay_ms` and `timer_period_ms`. **]**

**SRS_THREADPOOL_LINUX_42_046: [** `threadpool_timer_restart` shall release the timer lock by calling `srw_lock_ll_release_exclusive`. **]**

**SRS_THREADPOOL_LINUX_42_047: [** `threadpool_timer_restart` shall succeed and return 0. **]**

### threadpool_timer_cancel

```c
MOCKABLE_FUNCTION(, void, threadpool_timer_cancel, THANDLE(THREADPOOL_TIMER), timer);
```

`threadpool_timer_cancel` stops the timer and waits for its running callback. The timer can be started again with `threadpool_timer_restart`.

**SRS_THREADPOOL_LINUX_42_048: [** If `timer` is `NULL`, `threadpool_timer_cancel` shall return. **]**

**SRS_THREADPOOL_LINUX_42_049: [** `threadpool_timer_cancel` shall acquire the timer lock exclusively by calling `srw_lock_ll_acquire_exclusive`. **]**

**SRS_THREADPOOL_LINUX_42_050: [** `threadpool_timer_cancel` shall disarm the timer by calling `execution_engine_linux_cancel_timer`. **]**

**SRS_THREADPOOL_LINUX_42_051: [** `threadpool_timer_cancel` shall drop the callbacks that have not started and wait for the running callback by calling `execution_engine_linux_wait_for_work` with `cancel_pending` set to `true`. **]**

**SRS_THREADPOOL_LINUX_42_052: [** `threadpool_timer_cancel` shall release the timer lock by calling `srw_lock_ll_release_exclusive`. **]**

### threadpool_timer_dispose

```c
static void threadpool_timer_dispose(THREADPOOL_TIMER* timer);
```

`threadpool_timer_dispose` is called when the last reference to the timer is released.

**SRS_THREADPOOL_LINUX_42_053: [** `threadpool_timer_dispose` shall disarm the timer by calling `execution_engine_linux_cancel_timer`. **]**

**SRS_THREADPOOL_LINUX_42_054: [** `threadpool_timer_dispose` shall drop the callbacks that have not started and wait for the running callback by calling `execution_engine_linux_wait_for_work` with `cancel_pending` set to `true`. **]**

**SRS_THREADPOOL_LINUX_42_055: [** `threadpool_timer_dispose` shall close the timer by calling `execution_engine_linux_close_work`. **]**

**SRS_THREADPOOL_LINUX_42_056: [** `threadpool_timer_dispose` shall de-initialize the timer lock by calling `srw_lock_ll_deinit`. **]**
