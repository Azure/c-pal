# `execution_engine_linux` requirements

## Overview

`execution_engine_linux` is the Linux implementation of the `execution_engine` interface.

An execution engine is the thread pool shared by every `threadpool` created on it, the Linux equivalent of a Win32 `PTP_POOL`. It exposes a Linux-specific API modeled on the Win32 thread pool API, which `threadpool_linux` maps onto:

| `execution_engine_linux`                                        | Win32                                   |
|-----------------------------------------------------------------|-----------------------------------------|
| `execution_engine_linux_create_work`                            | `CreateThreadpoolWork`                  |
| `execution_engine_linux_submit_work`                            | `SubmitThreadpoolWork`                  |
| `execution_engine_linux_wait_for_work`                          | `WaitForThreadpoolWorkCallbacks`        |
| `execution_engine_linux_close_work`                             | `CloseThreadpoolWork`                   |
| `execution_engine_linux_disassociate_current_thread_from_work`  | `DisassociateCurrentThreadFromCallback` |
| `execution_engine_linux_create_timer`                           | `CreateThreadpoolTimer`                 |
| `execution_engine_linux_set_timer`                              | `SetThreadpoolTimer`                    |
| `execution_engine_linux_cancel_timer`                           | `SetThreadpoolTimer` with `NULL`        |
| `execution_engine_linux_wait_for_timer`                         | `WaitForThreadpoolTimerCallbacks`       |
| `execution_engine_linux_close_timer`                            | `CloseThreadpoolTimer`                  |
| `execution_engine_linux_disassociate_current_thread_from_timer` | `DisassociateCurrentThreadFromCallback` |

The execution engine is built from these modules:

| Module                      | Role                                                     |
|-----------------------------|----------------------------------------------------------|
| `doublylinkedlist`          | The ready queue of work objects.                         |
| `timer_queue_linux`         | The armed timers, ordered by due time.                   |
| `worker_pool_linux`         | The worker threads, idle worker threads and waking them. |
| `thread_count_policy_linux` | When to add a worker thread and when to retire one.      |
| `timer`                     | The current time (`CLOCK_MONOTONIC`, in milliseconds).   |

One mutex guards the state of the execution engine and of its worker pool. `internal_execution_engine_linux_run_next_work` and the free functions of the work objects are called with the mutex held.

### Work objects

A work object is created once (this can fail) and then submitted any number of times. Submitting never allocates and cannot fail: it increments the 64-bit pending count of the work object, which cannot overflow, and, when the count goes from 0 to 1, inserts the work object at the tail of the ready queue. A worker thread that takes a work object from the ready queue decrements its pending count and, if invocations are still pending, inserts it back at the tail before running the work function, so several invocations of the same work object can run concurrently, as with Win32.

Each work object counts its pending and running invocations. Waiting for a work object waits until both are 0, optionally dropping the invocations that have not started. Closing a work object frees it immediately if it has no pending and no running invocations, otherwise after the last one completes, so a work function can close its own work object.

A work function can disassociate its thread from its work object, like Win32 `DisassociateCurrentThreadFromCallback`: its invocation then no longer counts as running, so the work function can wait for or close its own work object, and threads waiting for the work object stop waiting for it. A thread-local variable records the work object whose work function runs on the current thread, and the worker thread does not access the work object again once its thread was disassociated from it.

### Timers

A timer contains a work object, which the manager thread submits when the timer is due. The armed timers are in a `timer_queue_linux`. Creating a timer reserves room for it in the timer queue (this can fail), so arming, re-arming and cancelling a timer cannot fail. A timer never fires before its start delay: its due time is computed from the current time rounded up to the next millisecond. A start delay of 0 makes the timer due immediately.

### Threads

The execution engine has a manager thread and between `min_thread_count` and the maximum number of worker threads (see `thread_count_policy_linux`). Worker threads run the work objects of the ready queue. Worker threads are never added on the submit path, so submitting stays infallible.

The manager thread:
- submits the due timers,
- retires the worker threads idle for too long,
- joins the retired worker threads,
- adds a worker thread when work in the ready queue makes no progress.

Linux gives no notification when a thread blocks, which is how Win32 adds threads immediately when callbacks block. Instead the manager thread adds a worker thread when work waits in the ready queue, no worker thread is idle and no work was taken from the ready queue for 50 ms.

Submitting work wakes the manager thread only when no worker thread is idle, a worker thread can be added and the manager thread is not already going to check the ready queue for progress.

### Lifetime

Work objects and timers must be closed before the last reference to the execution engine is released.

As with Win32 `CloseThreadpool`, the last reference can be released from a work function running on the execution engine. The execution engine can't wait for its own worker thread to exit, so the manager thread then tears down the execution engine once the worker threads exit, and exits. When the last reference is released from any other thread, `execution_engine_dec_ref` tears down the execution engine before returning.

## Exposed API

`execution_engine_linux` implements the `execution_engine` API and additionally exposes the following API:

```c
typedef struct EXECUTION_ENGINE_LINUX_WORK_TAG* EXECUTION_ENGINE_LINUX_WORK_HANDLE;
typedef struct EXECUTION_ENGINE_LINUX_TIMER_TAG* EXECUTION_ENGINE_LINUX_TIMER_HANDLE;
typedef void (*EXECUTION_ENGINE_LINUX_WORK_FUNCTION)(void* context);

MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_work, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
MOCKABLE_FUNCTION(, void, execution_engine_linux_submit_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work, bool, cancel_pending);
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, execution_engine_linux_create_timer, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
MOCKABLE_FUNCTION(, void, execution_engine_linux_set_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
MOCKABLE_FUNCTION(, void, execution_engine_linux_cancel_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, bool, cancel_pending);
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

### execution_engine_create

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_parameters);
```

`execution_engine_create` creates an execution engine and starts its threads.

**SRS_EXECUTION_ENGINE_LINUX_07_002: [** If `execution_engine_parameters` is `NULL`, `execution_engine_create` shall use the default `DEFAULT_MIN_THREAD_COUNT` and `DEFAULT_MAX_THREAD_COUNT` as parameters. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_003: [** `execution_engine_create` shall set the minimum number of threads to the `min_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_004: [** `execution_engine_create` shall set the maximum number of threads to the `max_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_005: [** If `max_thread_count` is non-zero and less than `min_thread_count`, `execution_engine_create` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_001: [** `execution_engine_create` shall allocate a new execution engine and on success shall return a non-`NULL` handle. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_178: [** `execution_engine_create` shall mark the execution engine as not stopping, with its teardown not deferred and the manager thread not watching the ready queue. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_179: [** `execution_engine_create` shall set the count of work objects in the ready queue to 0. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_001: [** `execution_engine_create` shall create the thread count policy by calling `thread_count_policy_create` with the minimum and the maximum number of threads. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_002: [** `execution_engine_create` shall create the timer queue by calling `timer_queue_create`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_003: [** `execution_engine_create` shall initialize the ready queue by calling `DList_InitializeListHead`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_006: [** `execution_engine_create` shall initialize the mutex by calling `pthread_mutex_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_008: [** `execution_engine_create` shall initialize a condition variable attribute by calling `pthread_condattr_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_009: [** `execution_engine_create` shall set the clock of the attribute to `CLOCK_MONOTONIC`, the clock of `timer_global_get_elapsed_ms`, by calling `pthread_condattr_setclock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_010: [** `execution_engine_create` shall initialize the condition variable that wakes the manager thread with the attribute by calling `pthread_cond_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_175: [** `execution_engine_create` shall destroy the attribute by calling `pthread_condattr_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_011: [** `execution_engine_create` shall initialize the condition variable signaled when work objects have no more pending and running invocations by calling `pthread_cond_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_013: [** `execution_engine_create` shall create the worker pool by calling `worker_pool_create` with the mutex, `internal_execution_engine_linux_run_next_work` and the execution engine. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_014: [** `execution_engine_create` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_015: [** `execution_engine_create` shall add `min_thread_count` worker threads by calling `worker_pool_add_worker`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_016: [** `execution_engine_create` shall release the mutex by calling `pthread_mutex_unlock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_017: [** `execution_engine_create` shall start the manager thread by calling `pthread_create` with `internal_execution_engine_linux_manager_thread_func` and the execution engine. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_018: [** If any error occurs after the worker pool was created, `execution_engine_create` shall stop the worker threads by calling `worker_pool_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_012: [** If any error occurs, `execution_engine_create` shall destroy what it created and initialized. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_006: [** If any error occurs, `execution_engine_create` shall fail and return `NULL`. **]**

### execution_engine_dec_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_dec_ref` releases a reference to the execution engine and tears it down when the last reference is released.

**SRS_EXECUTION_ENGINE_LINUX_07_007: [** If `execution_engine` is `NULL`, `execution_engine_dec_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_008: [** Otherwise `execution_engine_dec_ref` shall decrement the refcount. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_020: [** If the refcount reaches 0: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_021: [** `execution_engine_dec_ref` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_022: [** `execution_engine_dec_ref` shall mark the execution engine as stopping. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_023: [** If the current thread is running a work function of `execution_engine`, `execution_engine_dec_ref` shall mark the teardown as deferred to the manager thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_024: [** `execution_engine_dec_ref` shall wake the manager thread by calling `pthread_cond_signal`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_025: [** `execution_engine_dec_ref` shall release the mutex by calling `pthread_mutex_unlock`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_026: [** If the current thread is not running a work function of `execution_engine`: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_027: [** `execution_engine_dec_ref` shall wait for the manager thread to exit by calling `pthread_join`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_028: [** If `pthread_join` succeeds: **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_029: [** `execution_engine_dec_ref` shall stop the worker threads and wait for them to exit by calling `worker_pool_destroy`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_030: [** `execution_engine_dec_ref` shall destroy the condition variable signaled when work objects have no more pending and running invocations by calling `pthread_cond_destroy`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_031: [** `execution_engine_dec_ref` shall destroy the condition variable that wakes the manager thread by calling `pthread_cond_destroy`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_032: [** `execution_engine_dec_ref` shall destroy the mutex by calling `pthread_mutex_destroy`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_033: [** `execution_engine_dec_ref` shall destroy the timer queue by calling `timer_queue_destroy`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_034: [** `execution_engine_dec_ref` shall destroy the thread count policy by calling `thread_count_policy_destroy`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_035: [** `execution_engine_dec_ref` shall free the execution engine. **]**

### execution_engine_inc_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_inc_ref` adds a reference to the execution engine.

**SRS_EXECUTION_ENGINE_LINUX_07_010: [** If `execution_engine` is `NULL` then `execution_engine_inc_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_011: [** Otherwise `execution_engine_inc_ref` shall increment the reference count for `execution_engine`. **]**

### execution_engine_linux_get_parameters

```c
MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_linux_get_parameters` returns the parameters the execution engine was created with.

**SRS_EXECUTION_ENGINE_LINUX_07_012: [** If `execution_engine` is `NULL`, `execution_engine_linux_get_parameters` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_013: [** Otherwise, `execution_engine_linux_get_parameters` shall return the parameters in `EXECUTION_ENGINE`. **]**

### execution_engine_linux_create_work

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_work, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
```

`execution_engine_linux_create_work` creates a work object that calls `work_function` with `work_function_context` each time it is submitted.

**SRS_EXECUTION_ENGINE_LINUX_42_036: [** If `execution_engine` is `NULL`, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_037: [** If `work_function` is `NULL`, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_038: [** `execution_engine_linux_create_work` shall allocate a work object. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_042: [** `execution_engine_linux_create_work` shall store `execution_engine`, `work_function`, `work_function_context` and `internal_execution_engine_linux_free_work` as free function in the work object. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_043: [** `execution_engine_linux_create_work` shall set the pending count, the running count and the waiter count of the work object to 0. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_044: [** `execution_engine_linux_create_work` shall mark the work object as not closed. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_040: [** `execution_engine_linux_create_work` shall succeed and return the work object. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_041: [** If any error occurs, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

### internal_execution_engine_linux_free_work

```c
static void internal_execution_engine_linux_free_work(EXECUTION_ENGINE_LINUX_WORK* work);
```

`internal_execution_engine_linux_free_work` is the free function of the work objects created by `execution_engine_linux_create_work`.

**SRS_EXECUTION_ENGINE_LINUX_42_045: [** `internal_execution_engine_linux_free_work` shall free `work`. **]**

### execution_engine_linux_submit_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_submit_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_submit_work` queues one invocation of `work`, which cannot fail.

**SRS_EXECUTION_ENGINE_LINUX_42_046: [** If `work` is `NULL`, `execution_engine_linux_submit_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_047: [** `execution_engine_linux_submit_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_051: [** `execution_engine_linux_submit_work` shall increment the pending count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_052: [** If the pending count of `work` became 1: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_055: [** If the ready queue is empty, `execution_engine_linux_submit_work` shall record progress by calling `thread_count_policy_on_progress` with the current time obtained by calling `timer_global_get_elapsed_ms`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_056: [** `execution_engine_linux_submit_work` shall insert `work` at the tail of the ready queue by calling `DList_InsertTailList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_180: [** `execution_engine_linux_submit_work` shall increment the count of work objects in the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_057: [** `execution_engine_linux_submit_work` shall wake an idle worker thread by calling `worker_pool_wake_idle_worker`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_058: [** If `worker_pool_wake_idle_worker` returns `WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER` and the manager thread is not watching the ready queue: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_059: [** `execution_engine_linux_submit_work` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_060: [** If `thread_count_policy_can_add_thread` returns `true` for the thread count, `execution_engine_linux_submit_work` shall wake the manager thread by calling `pthread_cond_signal`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_049: [** `execution_engine_linux_submit_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### internal_execution_engine_linux_run_next_work

```c
static bool internal_execution_engine_linux_run_next_work(void* context);
```

`internal_execution_engine_linux_run_next_work` is the `WORKER_POOL_RUN_FUNCTION` of the worker pool, running the next work object of the ready queue, with the execution engine as `context`.

**SRS_EXECUTION_ENGINE_LINUX_42_061: [** If the ready queue is empty, `internal_execution_engine_linux_run_next_work` shall return `false`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_062: [** If the ready queue is not empty: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_063: [** `internal_execution_engine_linux_run_next_work` shall remove the work object at the head of the ready queue by calling `DList_RemoveHeadList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_181: [** `internal_execution_engine_linux_run_next_work` shall decrement the count of work objects in the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_064: [** `internal_execution_engine_linux_run_next_work` shall decrement the pending count of the work object. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_065: [** If the pending count of the work object is not 0: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_182: [** `internal_execution_engine_linux_run_next_work` shall insert the work object at the tail of the ready queue by calling `DList_InsertTailList`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_183: [** `internal_execution_engine_linux_run_next_work` shall increment the count of work objects in the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_066: [** If the ready queue is not empty: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_205: [** `internal_execution_engine_linux_run_next_work` shall wake an idle worker thread by calling `worker_pool_wake_idle_worker`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_206: [** If `worker_pool_wake_idle_worker` returns `WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER` and the manager thread is not watching the ready queue: **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_207: [** `internal_execution_engine_linux_run_next_work` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_208: [** If `thread_count_policy_can_add_thread` returns `true` for the thread count, `internal_execution_engine_linux_run_next_work` shall wake the manager thread by calling `pthread_cond_signal`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_067: [** `internal_execution_engine_linux_run_next_work` shall increment the running count of the work object. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_068: [** `internal_execution_engine_linux_run_next_work` shall record progress by calling `thread_count_policy_on_progress` with the current time obtained by calling `timer_global_get_elapsed_ms`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_069: [** `internal_execution_engine_linux_run_next_work` shall record the execution engine as the execution engine running a work function on the current thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_070: [** `internal_execution_engine_linux_run_next_work` shall record the work object as the work object running on the current thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_071: [** `internal_execution_engine_linux_run_next_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_072: [** `internal_execution_engine_linux_run_next_work` shall call the work function of the work object with its work function context. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_073: [** `internal_execution_engine_linux_run_next_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_074: [** `internal_execution_engine_linux_run_next_work` shall clear the execution engine running a work function on the current thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_075: [** If the work object is still the work object running on the current thread: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_076: [** `internal_execution_engine_linux_run_next_work` shall clear the work object running on the current thread. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_209: [** `internal_execution_engine_linux_run_next_work` shall decrement the running count of the work object. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_210: [** If the work object has no pending and no running invocations: **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_211: [** If the work object is closed, `internal_execution_engine_linux_run_next_work` shall call the free function of the work object. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_212: [** If the work object is not closed and the waiter count of the work object is not 0, `internal_execution_engine_linux_run_next_work` shall wake the waiting threads by calling `pthread_cond_broadcast`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_078: [** If the current thread was disassociated from the work object, `internal_execution_engine_linux_run_next_work` shall not access the work object, which can be freed by then. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_079: [** `internal_execution_engine_linux_run_next_work` shall return `true`. **]**

### execution_engine_linux_wait_for_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work, bool, cancel_pending);
```

`execution_engine_linux_wait_for_work` waits until `work` has no pending and no running invocations, first dropping the invocations that have not started if `cancel_pending` is `true`.

**SRS_EXECUTION_ENGINE_LINUX_42_084: [** If `work` is `NULL`, `execution_engine_linux_wait_for_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_086: [** `execution_engine_linux_wait_for_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_087: [** If `cancel_pending` is `true` and the pending count of `work` is not 0: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_088: [** `execution_engine_linux_wait_for_work` shall remove `work` from the ready queue by calling `DList_RemoveEntryList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_184: [** `execution_engine_linux_wait_for_work` shall decrement the count of work objects in the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_089: [** `execution_engine_linux_wait_for_work` shall set the pending count of `work` to 0. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_177: [** If `work` has no running invocations and the waiter count of `work` is not 0, `execution_engine_linux_wait_for_work` shall wake the threads waiting for `work` by calling `pthread_cond_broadcast`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_090: [** `execution_engine_linux_wait_for_work` shall increment the waiter count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_091: [** `execution_engine_linux_wait_for_work` shall wait by calling `pthread_cond_wait` until `work` has no pending and no running invocations. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_092: [** `execution_engine_linux_wait_for_work` shall decrement the waiter count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_093: [** `execution_engine_linux_wait_for_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_close_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_close_work` frees `work` once it has no pending and no running invocations, without waiting.

**SRS_EXECUTION_ENGINE_LINUX_42_094: [** If `work` is `NULL`, `execution_engine_linux_close_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_095: [** `execution_engine_linux_close_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_098: [** If `work` has no pending and no running invocations, `execution_engine_linux_close_work` shall call the free function of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_099: [** If `work` has pending or running invocations, `execution_engine_linux_close_work` shall mark `work` as closed. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_097: [** `execution_engine_linux_close_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_disassociate_current_thread_from_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_disassociate_current_thread_from_work` ends the invocation of `work` running on the current thread, if any, so that waiting for or closing `work` does not wait for the current thread.

**SRS_EXECUTION_ENGINE_LINUX_42_100: [** If `work` is `NULL`, `execution_engine_linux_disassociate_current_thread_from_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_102: [** If `work` is not the work object running on the current thread, `execution_engine_linux_disassociate_current_thread_from_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_103: [** `execution_engine_linux_disassociate_current_thread_from_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_104: [** `execution_engine_linux_disassociate_current_thread_from_work` shall clear the work object running on the current thread. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_225: [** `execution_engine_linux_disassociate_current_thread_from_work` shall decrement the running count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_226: [** If `work` has no pending and no running invocations: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_227: [** If `work` is closed, `execution_engine_linux_disassociate_current_thread_from_work` shall call the free function of `work`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_228: [** If `work` is not closed and the waiter count of `work` is not 0, `execution_engine_linux_disassociate_current_thread_from_work` shall wake the waiting threads by calling `pthread_cond_broadcast`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_106: [** `execution_engine_linux_disassociate_current_thread_from_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_create_timer

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, execution_engine_linux_create_timer, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
```

`execution_engine_linux_create_timer` creates a timer that is not armed and calls `work_function` with `work_function_context` each time it fires.

**SRS_EXECUTION_ENGINE_LINUX_42_107: [** If `execution_engine` is `NULL`, `execution_engine_linux_create_timer` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_108: [** If `work_function` is `NULL`, `execution_engine_linux_create_timer` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_109: [** `execution_engine_linux_create_timer` shall allocate a timer. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_193: [** `execution_engine_linux_create_timer` shall store `execution_engine`, `work_function`, `work_function_context` and `internal_execution_engine_linux_free_timer` as free function in the work object of the timer. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_194: [** `execution_engine_linux_create_timer` shall set the pending count, the running count and the waiter count of the work object of the timer to 0. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_195: [** `execution_engine_linux_create_timer` shall mark the work object of the timer as not closed. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_111: [** `execution_engine_linux_create_timer` shall initialize the timer queue entry of the timer by calling `timer_queue_entry_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_112: [** `execution_engine_linux_create_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_113: [** `execution_engine_linux_create_timer` shall reserve room for the timer in the timer queue by calling `timer_queue_reserve`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_114: [** `execution_engine_linux_create_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_115: [** `execution_engine_linux_create_timer` shall succeed and return the timer. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_116: [** If any error occurs, `execution_engine_linux_create_timer` shall free the timer, fail and return `NULL`. **]**

### internal_execution_engine_linux_free_timer

```c
static void internal_execution_engine_linux_free_timer(EXECUTION_ENGINE_LINUX_WORK* work);
```

`internal_execution_engine_linux_free_timer` is the free function of the work objects of timers.

**SRS_EXECUTION_ENGINE_LINUX_42_117: [** `internal_execution_engine_linux_free_timer` shall give back the room reserved for the timer in the timer queue by calling `timer_queue_unreserve`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_118: [** `internal_execution_engine_linux_free_timer` shall free the timer. **]**

### execution_engine_linux_set_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_set_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`execution_engine_linux_set_timer` arms `timer` to fire after `start_delay_ms` and then every `timer_period_ms` (only once if `timer_period_ms` is 0), replacing any previous arming, which cannot fail.

**SRS_EXECUTION_ENGINE_LINUX_42_119: [** If `timer` is `NULL`, `execution_engine_linux_set_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_120: [** `execution_engine_linux_set_timer` shall get the current time rounded up to the next millisecond by calling `timer_global_get_elapsed_ms`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_121: [** `execution_engine_linux_set_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_124: [** `execution_engine_linux_set_timer` shall arm `timer` with the current time plus `start_delay_ms` as due time and `timer_period_ms` as period by calling `timer_queue_arm`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_125: [** If `timer_queue_arm` returns `true`, `execution_engine_linux_set_timer` shall wake the manager thread by calling `pthread_cond_signal`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_126: [** `execution_engine_linux_set_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_cancel_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_cancel_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

`execution_engine_linux_cancel_timer` disarms `timer`, without dropping or waiting for the invocations already submitted.

**SRS_EXECUTION_ENGINE_LINUX_42_127: [** If `timer` is `NULL`, `execution_engine_linux_cancel_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_128: [** `execution_engine_linux_cancel_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_129: [** `execution_engine_linux_cancel_timer` shall disarm `timer` by calling `timer_queue_disarm`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_130: [** `execution_engine_linux_cancel_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_wait_for_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, bool, cancel_pending);
```

`execution_engine_linux_wait_for_timer` waits until `timer` has no pending and no running invocations, first dropping the invocations that have not started if `cancel_pending` is `true`.

**SRS_EXECUTION_ENGINE_LINUX_42_131: [** If `timer` is `NULL`, `execution_engine_linux_wait_for_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_213: [** `execution_engine_linux_wait_for_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_214: [** If `cancel_pending` is `true` and the pending count of the work object of `timer` is not 0: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_215: [** `execution_engine_linux_wait_for_timer` shall remove the work object of `timer` from the ready queue by calling `DList_RemoveEntryList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_216: [** `execution_engine_linux_wait_for_timer` shall decrement the count of work objects in the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_217: [** `execution_engine_linux_wait_for_timer` shall set the pending count of the work object of `timer` to 0. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_218: [** If the work object of `timer` has no running invocations and the waiter count of the work object of `timer` is not 0, `execution_engine_linux_wait_for_timer` shall wake the threads waiting for the work object of `timer` by calling `pthread_cond_broadcast`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_219: [** `execution_engine_linux_wait_for_timer` shall increment the waiter count of the work object of `timer`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_220: [** `execution_engine_linux_wait_for_timer` shall wait by calling `pthread_cond_wait` until the work object of `timer` has no pending and no running invocations. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_221: [** `execution_engine_linux_wait_for_timer` shall decrement the waiter count of the work object of `timer`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_222: [** `execution_engine_linux_wait_for_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_close_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

`execution_engine_linux_close_timer` disarms `timer` and frees it once it has no pending and no running invocations, without waiting.

**SRS_EXECUTION_ENGINE_LINUX_42_133: [** If `timer` is `NULL`, `execution_engine_linux_close_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_134: [** `execution_engine_linux_close_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_135: [** `execution_engine_linux_close_timer` shall disarm `timer` by calling `timer_queue_disarm`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_223: [** If the work object of `timer` has no pending and no running invocations, `execution_engine_linux_close_timer` shall call the free function of the work object of `timer`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_224: [** If the work object of `timer` has pending or running invocations, `execution_engine_linux_close_timer` shall mark the work object of `timer` as closed. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_137: [** `execution_engine_linux_close_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_disassociate_current_thread_from_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

`execution_engine_linux_disassociate_current_thread_from_timer` ends the invocation of `timer` running on the current thread, if any, so that waiting for or closing `timer` does not wait for the current thread.

**SRS_EXECUTION_ENGINE_LINUX_42_138: [** If `timer` is `NULL`, `execution_engine_linux_disassociate_current_thread_from_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_229: [** If the work object of `timer` is not the work object running on the current thread, `execution_engine_linux_disassociate_current_thread_from_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_230: [** `execution_engine_linux_disassociate_current_thread_from_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_231: [** `execution_engine_linux_disassociate_current_thread_from_timer` shall clear the work object running on the current thread. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_232: [** `execution_engine_linux_disassociate_current_thread_from_timer` shall decrement the running count of the work object of `timer`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_233: [** If the work object of `timer` has no pending and no running invocations: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_234: [** If the work object of `timer` is closed, `execution_engine_linux_disassociate_current_thread_from_timer` shall call the free function of the work object of `timer`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_235: [** If the work object of `timer` is not closed and the waiter count of the work object of `timer` is not 0, `execution_engine_linux_disassociate_current_thread_from_timer` shall wake the waiting threads by calling `pthread_cond_broadcast`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_236: [** `execution_engine_linux_disassociate_current_thread_from_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### internal_execution_engine_linux_manager_thread_func

```c
static void* internal_execution_engine_linux_manager_thread_func(void* arg);
```

`internal_execution_engine_linux_manager_thread_func` is the function of the manager thread, with the execution engine as `arg`.

**SRS_EXECUTION_ENGINE_LINUX_42_140: [** `internal_execution_engine_linux_manager_thread_func` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_141: [** Until the execution engine is stopping: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_142: [** `internal_execution_engine_linux_manager_thread_func` shall get the current time by calling `timer_global_get_elapsed_ms`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_155: [** For each timer returned by `timer_queue_pop_due` with the current time: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_196: [** `internal_execution_engine_linux_manager_thread_func` shall increment the pending count of the work object of the timer. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_197: [** If the pending count of the work object of the timer became 1: **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_198: [** If the ready queue is empty, `internal_execution_engine_linux_manager_thread_func` shall record progress by calling `thread_count_policy_on_progress` with the current time obtained by calling `timer_global_get_elapsed_ms`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_199: [** `internal_execution_engine_linux_manager_thread_func` shall insert the work object of the timer at the tail of the ready queue by calling `DList_InsertTailList`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_200: [** `internal_execution_engine_linux_manager_thread_func` shall increment the count of work objects in the ready queue. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_201: [** `internal_execution_engine_linux_manager_thread_func` shall wake an idle worker thread by calling `worker_pool_wake_idle_worker`. **]**

    - **SRS_EXECUTION_ENGINE_LINUX_42_202: [** If `worker_pool_wake_idle_worker` returns `WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER` and the manager thread is not watching the ready queue: **]**

      - **SRS_EXECUTION_ENGINE_LINUX_42_203: [** `internal_execution_engine_linux_manager_thread_func` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

      - **SRS_EXECUTION_ENGINE_LINUX_42_204: [** If `thread_count_policy_can_add_thread` returns `true` for the thread count, `internal_execution_engine_linux_manager_thread_func` shall wake the manager thread by calling `pthread_cond_signal`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_162: [** `internal_execution_engine_linux_manager_thread_func` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_163: [** While the time to retire a worker thread obtained by calling `thread_count_policy_get_retire_thread_time` with the counts and the current time is not later than the current time: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_164: [** `internal_execution_engine_linux_manager_thread_func` shall retire the worker thread idle the longest by calling `worker_pool_retire_oldest_idle_worker`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_165: [** `internal_execution_engine_linux_manager_thread_func` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_145: [** `internal_execution_engine_linux_manager_thread_func` shall join the retired worker threads by calling `worker_pool_join_retired_workers`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_146: [** `internal_execution_engine_linux_manager_thread_func` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_147: [** `internal_execution_engine_linux_manager_thread_func` shall get the time to add a worker thread by calling `thread_count_policy_get_add_thread_time` with the number of work objects in the ready queue and the counts. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_148: [** If the time to add a worker thread is not later than the current time: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_166: [** `internal_execution_engine_linux_manager_thread_func` shall add a worker thread by calling `worker_pool_add_worker`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_167: [** If `worker_pool_add_worker` fails, `internal_execution_engine_linux_manager_thread_func` shall call `thread_count_policy_on_add_thread_failed` with the current time. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_168: [** If `worker_pool_add_worker` succeeds, `internal_execution_engine_linux_manager_thread_func` shall record progress by calling `thread_count_policy_on_progress` with the current time. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_149: [** If the time to add a worker thread is later than the current time: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_169: [** `internal_execution_engine_linux_manager_thread_func` shall record that the manager thread is watching the ready queue if the time to add a worker thread is not `THREAD_COUNT_POLICY_NEVER`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_170: [** `internal_execution_engine_linux_manager_thread_func` shall get the time to retire a worker thread by calling `thread_count_policy_get_retire_thread_time` with the counts and the current time. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_171: [** `internal_execution_engine_linux_manager_thread_func` shall get the due time of the first timer by calling `timer_queue_get_next_due_time`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_172: [** `internal_execution_engine_linux_manager_thread_func` shall compute the deadline as the earliest of the time to add a worker thread, the time to retire a worker thread and the due time of the first timer. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_173: [** If the deadline is `THREAD_COUNT_POLICY_NEVER`, `internal_execution_engine_linux_manager_thread_func` shall wait by calling `pthread_cond_wait`. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_174: [** If the deadline is not `THREAD_COUNT_POLICY_NEVER`, `internal_execution_engine_linux_manager_thread_func` shall wait until the deadline by calling `pthread_cond_timedwait`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_150: [** `internal_execution_engine_linux_manager_thread_func` shall release the mutex by calling `pthread_mutex_unlock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_151: [** If the teardown is deferred to the manager thread: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_152: [** `internal_execution_engine_linux_manager_thread_func` shall detach the manager thread by calling `pthread_detach`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_186: [** `internal_execution_engine_linux_manager_thread_func` shall stop the worker threads and wait for them to exit by calling `worker_pool_destroy`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_187: [** `internal_execution_engine_linux_manager_thread_func` shall destroy the condition variable signaled when work objects have no more pending and running invocations by calling `pthread_cond_destroy`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_188: [** `internal_execution_engine_linux_manager_thread_func` shall destroy the condition variable that wakes the manager thread by calling `pthread_cond_destroy`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_189: [** `internal_execution_engine_linux_manager_thread_func` shall destroy the mutex by calling `pthread_mutex_destroy`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_190: [** `internal_execution_engine_linux_manager_thread_func` shall destroy the timer queue by calling `timer_queue_destroy`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_191: [** `internal_execution_engine_linux_manager_thread_func` shall destroy the thread count policy by calling `thread_count_policy_destroy`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_192: [** `internal_execution_engine_linux_manager_thread_func` shall free the execution engine. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_154: [** `internal_execution_engine_linux_manager_thread_func` shall return `NULL`. **]**
