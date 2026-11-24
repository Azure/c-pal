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
| `min_heap`                  | The armed timers, ordered by due time.                   |
| `worker_pool_linux`         | The worker threads, idle worker threads and waking them. |
| `thread_count_policy_linux` | When to add a worker thread and when to retire one.      |
| `timer`                     | The current time (`CLOCK_MONOTONIC`, in milliseconds).   |

One mutex guards the state of the execution engine and of its worker pool.

### Work objects

A work object is created once (this can fail) and then submitted any number of times. Submitting never allocates and cannot fail: it increments the pending count of the work object and, when the count goes from 0 to 1, inserts the work object at the tail of the ready queue. A worker thread that takes a work object from the ready queue decrements its pending count and, if invocations are still pending, inserts it back at the tail before running the work function, so several invocations of the same work object can run concurrently, as with Win32.

Each work object counts its pending and running invocations. Waiting for a work object waits until both are 0, optionally dropping the invocations that have not started. Closing a work object frees it immediately if it has no pending and no running invocations, otherwise after the last one completes, so a work function can close its own work object.

A work function can disassociate its thread from its work object, like Win32 `DisassociateCurrentThreadFromCallback`: its invocation then no longer counts as running, so the work function can wait for or close its own work object, and threads waiting for the work object stop waiting for it. A thread-local variable records the work object whose work function runs on the current thread, and the worker thread does not access the work object again once its thread was disassociated from it.

### Timers

A timer contains a work object, which the manager thread submits when the timer is due. The armed timers are in a `min_heap` ordered by due time. Creating a timer reserves room for it in the heap (this can fail), so arming, re-arming and cancelling a timer cannot fail. A timer never fires before its start delay: its due time is computed from the current time rounded up to the next millisecond. A start delay of 0 makes the timer due immediately.

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

**SRS_EXECUTION_ENGINE_LINUX_42_001: [** `execution_engine_create` shall create the thread count policy by calling `thread_count_policy_create` with the minimum and the maximum number of threads. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_002: [** `execution_engine_create` shall create the timer heap by calling `min_heap_create`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_003: [** `execution_engine_create` shall initialize the ready queue by calling `DList_InitializeListHead`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_004: [** `execution_engine_create` shall initialize the mutex and the condition variables. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_005: [** `execution_engine_create` shall start the threads. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_006: [** If any error occurs, `execution_engine_create` shall fail and return `NULL`. **]**

### init_synchronization

```c
static int init_synchronization(EXECUTION_ENGINE* execution_engine);
```

`init_synchronization` initializes the mutex guarding the execution engine state, the condition variable that wakes the manager thread and the condition variable signaled when work objects have no more pending and running invocations. The manager thread waits with a deadline on `CLOCK_MONOTONIC`, the clock of `timer_global_get_elapsed_ms`.

**SRS_EXECUTION_ENGINE_LINUX_42_006: [** `init_synchronization` shall initialize the mutex by calling `pthread_mutex_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_007: [** `init_synchronization` shall initialize the condition variable that wakes the manager thread. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_011: [** `init_synchronization` shall initialize the condition variable signaled when work objects have no more pending and running invocations by calling `pthread_cond_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_012: [** If any error occurs, `init_synchronization` shall destroy what it initialized and fail. **]**

### init_manager_cond

```c
static int init_manager_cond(EXECUTION_ENGINE* execution_engine);
```

**SRS_EXECUTION_ENGINE_LINUX_42_008: [** `init_manager_cond` shall initialize a condition variable attribute by calling `pthread_condattr_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_009: [** `init_manager_cond` shall set the clock of the attribute to `CLOCK_MONOTONIC` by calling `pthread_condattr_setclock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_010: [** `init_manager_cond` shall initialize the condition variable that wakes the manager thread with the attribute by calling `pthread_cond_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_175: [** `init_manager_cond` shall destroy the attribute by calling `pthread_condattr_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_176: [** If any error occurs, `init_manager_cond` shall fail. **]**

### start_threads

```c
static int start_threads(EXECUTION_ENGINE* execution_engine);
```

**SRS_EXECUTION_ENGINE_LINUX_42_013: [** `start_threads` shall create the worker pool by calling `worker_pool_create` with the mutex, `run_next_work` and the execution engine. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_014: [** `start_threads` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_015: [** `start_threads` shall add `min_thread_count` worker threads by calling `worker_pool_add_worker`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_016: [** `start_threads` shall release the mutex by calling `pthread_mutex_unlock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_017: [** `start_threads` shall start the manager thread by calling `pthread_create` with `manager_thread_func` and the execution engine. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_018: [** If any error occurs after the worker pool was created, `start_threads` shall stop the worker threads by calling `worker_pool_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_019: [** If any error occurs, `start_threads` shall fail. **]**

### execution_engine_dec_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

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

  - **SRS_EXECUTION_ENGINE_LINUX_42_028: [** `execution_engine_dec_ref` shall tear down the execution engine. **]**

### teardown

```c
static void teardown(EXECUTION_ENGINE* execution_engine);
```

`teardown` runs after the manager thread stopped, without holding the mutex.

**SRS_EXECUTION_ENGINE_LINUX_42_029: [** `teardown` shall stop the worker threads and wait for them to exit by calling `worker_pool_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_030: [** `teardown` shall destroy the condition variable signaled when work objects have no more pending and running invocations by calling `pthread_cond_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_031: [** `teardown` shall destroy the condition variable that wakes the manager thread by calling `pthread_cond_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_032: [** `teardown` shall destroy the mutex by calling `pthread_mutex_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_033: [** `teardown` shall destroy the timer heap by calling `min_heap_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_034: [** `teardown` shall destroy the thread count policy by calling `thread_count_policy_destroy`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_035: [** `teardown` shall free the execution engine. **]**

### execution_engine_inc_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

**SRS_EXECUTION_ENGINE_LINUX_07_010: [** If `execution_engine` is `NULL` then `execution_engine_inc_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_011: [** Otherwise `execution_engine_inc_ref` shall increment the reference count for `execution_engine`. **]**

### execution_engine_linux_get_parameters

```c
MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);
```

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

**SRS_EXECUTION_ENGINE_LINUX_42_039: [** `execution_engine_linux_create_work` shall initialize the work object with `free_work` as its free function. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_040: [** `execution_engine_linux_create_work` shall succeed and return the work object. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_041: [** If any error occurs, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

### init_work

```c
static void init_work(EXECUTION_ENGINE_LINUX_WORK* work, EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION work_function, void* work_function_context, FREE_WORK_FUNCTION free_function);
```

**SRS_EXECUTION_ENGINE_LINUX_42_042: [** `init_work` shall store `execution_engine`, `work_function`, `work_function_context` and `free_function` in `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_043: [** `init_work` shall set the pending count, the running count and the waiter count of `work` to 0. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_044: [** `init_work` shall mark `work` as not closed. **]**

### free_work

```c
static void free_work(EXECUTION_ENGINE_LINUX_WORK* work);
```

**SRS_EXECUTION_ENGINE_LINUX_42_045: [** `free_work` shall free `work`. **]**

### execution_engine_linux_submit_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_submit_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_submit_work` queues one invocation of `work`. It cannot fail.

**SRS_EXECUTION_ENGINE_LINUX_42_046: [** If `work` is `NULL`, `execution_engine_linux_submit_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_047: [** `execution_engine_linux_submit_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_048: [** `execution_engine_linux_submit_work` shall submit `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_049: [** `execution_engine_linux_submit_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### submit_work_locked

```c
static void submit_work_locked(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work);
```

`submit_work_locked` is called with the mutex held. The pending count is 64-bit, so it cannot overflow.

**SRS_EXECUTION_ENGINE_LINUX_42_051: [** `submit_work_locked` shall increment the pending count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_052: [** If the pending count of `work` became 1: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_053: [** `submit_work_locked` shall insert `work` in the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_054: [** `submit_work_locked` shall dispatch the ready queue. **]**

### enqueue

```c
static void enqueue(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work);
```

`enqueue` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_055: [** If the ready queue is empty, `enqueue` shall record progress by calling `thread_count_policy_on_progress` with the current time obtained by calling `timer_global_get_elapsed_ms`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_056: [** `enqueue` shall insert `work` at the tail of the ready queue by calling `DList_InsertTailList`. **]**

### dispatch

```c
static void dispatch(EXECUTION_ENGINE* execution_engine);
```

`dispatch` makes sure a thread takes care of the work in the ready queue. It is called with the mutex held and the ready queue not empty.

**SRS_EXECUTION_ENGINE_LINUX_42_057: [** `dispatch` shall wake an idle worker thread by calling `worker_pool_wake_idle_worker`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_058: [** If `worker_pool_wake_idle_worker` returns `WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER` and the manager thread is not watching the ready queue: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_059: [** `dispatch` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_060: [** If `thread_count_policy_can_add_thread` returns `true` for the thread count, `dispatch` shall wake the manager thread by calling `pthread_cond_signal`. **]**

### run_next_work

```c
static bool run_next_work(void* context);
```

`run_next_work` is the `WORKER_POOL_RUN_FUNCTION` of the worker pool, called on a worker thread with the mutex held. `context` is the execution engine.

**SRS_EXECUTION_ENGINE_LINUX_42_061: [** If the ready queue is empty, `run_next_work` shall return `false`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_062: [** If the ready queue is not empty: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_063: [** `run_next_work` shall remove the work object at the head of the ready queue by calling `DList_RemoveHeadList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_064: [** `run_next_work` shall decrement the pending count of the work object. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_065: [** If the pending count of the work object is not 0, `run_next_work` shall insert the work object at the tail of the ready queue by calling `DList_InsertTailList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_066: [** If the ready queue is not empty, `run_next_work` shall dispatch the ready queue. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_067: [** `run_next_work` shall increment the running count of the work object. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_068: [** `run_next_work` shall record progress by calling `thread_count_policy_on_progress` with the current time obtained by calling `timer_global_get_elapsed_ms`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_069: [** `run_next_work` shall record the execution engine as the execution engine running a work function on the current thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_070: [** `run_next_work` shall record the work object as the work object running on the current thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_071: [** `run_next_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_072: [** `run_next_work` shall call the work function of the work object with its work function context. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_073: [** `run_next_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_074: [** `run_next_work` shall clear the execution engine running a work function on the current thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_075: [** If the work object is still the work object running on the current thread: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_076: [** `run_next_work` shall clear the work object running on the current thread. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_077: [** `run_next_work` shall end the invocation of the work object. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_078: [** If the current thread was disassociated from the work object, `run_next_work` shall not access the work object, which can be freed by then. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_079: [** `run_next_work` shall return `true`. **]**

### end_invocation

```c
static void end_invocation(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work);
```

`end_invocation` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_080: [** `end_invocation` shall decrement the running count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_081: [** If `work` has no pending and no running invocations: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_082: [** If `work` is closed, `end_invocation` shall call the free function of `work`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_083: [** If `work` is not closed and the waiter count of `work` is not 0, `end_invocation` shall wake the waiting threads by calling `pthread_cond_broadcast`. **]**

### execution_engine_linux_wait_for_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work, bool, cancel_pending);
```

`execution_engine_linux_wait_for_work` waits until `work` has no pending and no running invocations. If `cancel_pending` is `true`, it first drops the invocations that have not started.

**SRS_EXECUTION_ENGINE_LINUX_42_084: [** If `work` is `NULL`, `execution_engine_linux_wait_for_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_085: [** `execution_engine_linux_wait_for_work` shall wait for the invocations of `work`. **]**

### wait_for_invocations

```c
static void wait_for_invocations(EXECUTION_ENGINE_LINUX_WORK* work, bool cancel_pending);
```

**SRS_EXECUTION_ENGINE_LINUX_42_086: [** `wait_for_invocations` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_087: [** If `cancel_pending` is `true` and the pending count of `work` is not 0: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_088: [** `wait_for_invocations` shall remove `work` from the ready queue by calling `DList_RemoveEntryList`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_089: [** `wait_for_invocations` shall set the pending count of `work` to 0. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_177: [** If `work` has no running invocations and the waiter count of `work` is not 0, `wait_for_invocations` shall wake the threads waiting for `work` by calling `pthread_cond_broadcast`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_090: [** `wait_for_invocations` shall increment the waiter count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_091: [** `wait_for_invocations` shall wait by calling `pthread_cond_wait` until `work` has no pending and no running invocations. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_092: [** `wait_for_invocations` shall decrement the waiter count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_093: [** `wait_for_invocations` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_close_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_close_work` frees `work` once it has no pending and no running invocations. It does not wait.

**SRS_EXECUTION_ENGINE_LINUX_42_094: [** If `work` is `NULL`, `execution_engine_linux_close_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_095: [** `execution_engine_linux_close_work` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_096: [** `execution_engine_linux_close_work` shall close `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_097: [** `execution_engine_linux_close_work` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### close_work_locked

```c
static void close_work_locked(EXECUTION_ENGINE_LINUX_WORK* work);
```

`close_work_locked` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_098: [** If `work` has no pending and no running invocations, `close_work_locked` shall call the free function of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_099: [** If `work` has pending or running invocations, `close_work_locked` shall mark `work` as closed. **]**

### execution_engine_linux_disassociate_current_thread_from_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

When called from the work function of `work`, `execution_engine_linux_disassociate_current_thread_from_work` ends the invocation of `work` running on the current thread, so that waiting for `work` or closing it does not wait for the current thread. Otherwise it does nothing.

**SRS_EXECUTION_ENGINE_LINUX_42_100: [** If `work` is `NULL`, `execution_engine_linux_disassociate_current_thread_from_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_101: [** `execution_engine_linux_disassociate_current_thread_from_work` shall disassociate the current thread from `work`. **]**

### disassociate_current_thread

```c
static void disassociate_current_thread(EXECUTION_ENGINE_LINUX_WORK* work);
```

**SRS_EXECUTION_ENGINE_LINUX_42_102: [** If `work` is not the work object running on the current thread, `disassociate_current_thread` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_103: [** `disassociate_current_thread` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_104: [** `disassociate_current_thread` shall clear the work object running on the current thread. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_105: [** `disassociate_current_thread` shall end the invocation of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_106: [** `disassociate_current_thread` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_create_timer

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, execution_engine_linux_create_timer, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
```

`execution_engine_linux_create_timer` creates a timer that is not armed. The timer calls `work_function` with `work_function_context` each time it fires.

**SRS_EXECUTION_ENGINE_LINUX_42_107: [** If `execution_engine` is `NULL`, `execution_engine_linux_create_timer` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_108: [** If `work_function` is `NULL`, `execution_engine_linux_create_timer` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_109: [** `execution_engine_linux_create_timer` shall allocate a timer. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_110: [** `execution_engine_linux_create_timer` shall initialize the work object of the timer with `free_timer` as its free function. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_111: [** `execution_engine_linux_create_timer` shall initialize the timer heap entry of the timer by calling `min_heap_entry_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_112: [** `execution_engine_linux_create_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_113: [** `execution_engine_linux_create_timer` shall reserve room for the timer in the timer heap by calling `min_heap_reserve`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_114: [** `execution_engine_linux_create_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_115: [** `execution_engine_linux_create_timer` shall succeed and return the timer. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_116: [** If any error occurs, `execution_engine_linux_create_timer` shall free the timer, fail and return `NULL`. **]**

### free_timer

```c
static void free_timer(EXECUTION_ENGINE_LINUX_WORK* work);
```

`free_timer` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_117: [** `free_timer` shall release the room reserved for the timer in the timer heap by calling `min_heap_unreserve`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_118: [** `free_timer` shall free the timer. **]**

### execution_engine_linux_set_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_set_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`execution_engine_linux_set_timer` arms `timer` to fire after `start_delay_ms`, and then every `timer_period_ms` if `timer_period_ms` is not 0. It replaces any previous setting of `timer`. It cannot fail.

**SRS_EXECUTION_ENGINE_LINUX_42_119: [** If `timer` is `NULL`, `execution_engine_linux_set_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_120: [** `execution_engine_linux_set_timer` shall get the current time rounded up to the next millisecond by calling `timer_global_get_elapsed_ms`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_121: [** `execution_engine_linux_set_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_122: [** `execution_engine_linux_set_timer` shall remove `timer` from the timer heap by calling `min_heap_remove`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_123: [** `execution_engine_linux_set_timer` shall store `timer_period_ms` as the period of `timer`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_124: [** `execution_engine_linux_set_timer` shall insert `timer` in the timer heap with the current time plus `start_delay_ms` as due time by calling `min_heap_insert`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_125: [** If `timer` is the first timer of the timer heap obtained by calling `min_heap_peek`, `execution_engine_linux_set_timer` shall wake the manager thread by calling `pthread_cond_signal`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_126: [** `execution_engine_linux_set_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_cancel_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_cancel_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

`execution_engine_linux_cancel_timer` disarms `timer`. Invocations already submitted still run; `execution_engine_linux_wait_for_timer` waits for them or drops them.

**SRS_EXECUTION_ENGINE_LINUX_42_127: [** If `timer` is `NULL`, `execution_engine_linux_cancel_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_128: [** `execution_engine_linux_cancel_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_129: [** `execution_engine_linux_cancel_timer` shall remove `timer` from the timer heap by calling `min_heap_remove`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_130: [** `execution_engine_linux_cancel_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_wait_for_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer, bool, cancel_pending);
```

`execution_engine_linux_wait_for_timer` waits until `timer` has no pending and no running invocations. If `cancel_pending` is `true`, it first drops the invocations that have not started.

**SRS_EXECUTION_ENGINE_LINUX_42_131: [** If `timer` is `NULL`, `execution_engine_linux_wait_for_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_132: [** `execution_engine_linux_wait_for_timer` shall wait for the invocations of the work object of `timer`. **]**

### execution_engine_linux_close_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

`execution_engine_linux_close_timer` disarms `timer` and frees it once it has no pending and no running invocations. It does not wait.

**SRS_EXECUTION_ENGINE_LINUX_42_133: [** If `timer` is `NULL`, `execution_engine_linux_close_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_134: [** `execution_engine_linux_close_timer` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_135: [** `execution_engine_linux_close_timer` shall remove `timer` from the timer heap by calling `min_heap_remove`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_136: [** `execution_engine_linux_close_timer` shall close the work object of `timer`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_137: [** `execution_engine_linux_close_timer` shall release the mutex by calling `pthread_mutex_unlock`. **]**

### execution_engine_linux_disassociate_current_thread_from_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_timer, EXECUTION_ENGINE_LINUX_TIMER_HANDLE, timer);
```

When called from the work function of `timer`, `execution_engine_linux_disassociate_current_thread_from_timer` ends the invocation of `timer` running on the current thread, so that waiting for `timer` or closing it does not wait for the current thread. Otherwise it does nothing.

**SRS_EXECUTION_ENGINE_LINUX_42_138: [** If `timer` is `NULL`, `execution_engine_linux_disassociate_current_thread_from_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_139: [** `execution_engine_linux_disassociate_current_thread_from_timer` shall disassociate the current thread from the work object of `timer`. **]**

### manager_thread_func

```c
static void* manager_thread_func(void* arg);
```

`manager_thread_func` is the function of the manager thread. `arg` is the execution engine.

**SRS_EXECUTION_ENGINE_LINUX_42_140: [** `manager_thread_func` shall acquire the mutex by calling `pthread_mutex_lock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_141: [** Until the execution engine is stopping: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_142: [** `manager_thread_func` shall get the current time by calling `timer_global_get_elapsed_ms`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_143: [** `manager_thread_func` shall submit the due timers. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_144: [** `manager_thread_func` shall retire the worker threads idle for too long. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_145: [** `manager_thread_func` shall join the retired worker threads by calling `worker_pool_join_retired_workers`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_146: [** `manager_thread_func` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_147: [** `manager_thread_func` shall get the time to add a worker thread by calling `thread_count_policy_get_add_thread_time` with the number of work objects in the ready queue and the counts. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_148: [** If the time to add a worker thread is not later than the current time, `manager_thread_func` shall add a worker thread. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_149: [** If the time to add a worker thread is later than the current time, `manager_thread_func` shall wait for the next event. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_150: [** `manager_thread_func` shall release the mutex by calling `pthread_mutex_unlock`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_151: [** If the teardown is deferred to the manager thread: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_152: [** `manager_thread_func` shall detach the manager thread by calling `pthread_detach`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_153: [** `manager_thread_func` shall tear down the execution engine. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_154: [** `manager_thread_func` shall return `NULL`. **]**

### submit_due_timers

```c
static void submit_due_timers(EXECUTION_ENGINE* execution_engine, uint64_t now);
```

`submit_due_timers` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_155: [** For each timer whose due time is not later than `now`, in due time order, as obtained by calling `min_heap_peek`: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_156: [** `submit_due_timers` shall remove the timer from the timer heap by calling `min_heap_remove`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_157: [** If the period of the timer is not 0: **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_158: [** `submit_due_timers` shall compute the next due time as the due time plus the period. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_159: [** If the next due time is not later than `now`, `submit_due_timers` shall use `now` plus the period as the next due time. **]**

  - **SRS_EXECUTION_ENGINE_LINUX_42_160: [** `submit_due_timers` shall insert the timer in the timer heap with the next due time by calling `min_heap_insert`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_161: [** `submit_due_timers` shall submit the work object of the timer. **]**

### retire_idle_workers

```c
static void retire_idle_workers(EXECUTION_ENGINE* execution_engine, uint64_t now);
```

`retire_idle_workers` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_162: [** `retire_idle_workers` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_163: [** While the time to retire a worker thread obtained by calling `thread_count_policy_get_retire_thread_time` with the counts and `now` is not later than `now`: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_164: [** `retire_idle_workers` shall retire the worker thread idle the longest by calling `worker_pool_retire_oldest_idle_worker`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_165: [** `retire_idle_workers` shall get the counts of the worker pool by calling `worker_pool_get_counts`. **]**

### add_worker

```c
static void add_worker(EXECUTION_ENGINE* execution_engine, uint64_t now);
```

`add_worker` is called with the mutex held.

**SRS_EXECUTION_ENGINE_LINUX_42_166: [** `add_worker` shall add a worker thread by calling `worker_pool_add_worker`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_167: [** If `worker_pool_add_worker` fails, `add_worker` shall call `thread_count_policy_on_add_thread_failed` with `now`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_168: [** If `worker_pool_add_worker` succeeds, `add_worker` shall record progress by calling `thread_count_policy_on_progress` with `now`. **]**

### wait_for_next_event

```c
static void wait_for_next_event(EXECUTION_ENGINE* execution_engine, const WORKER_POOL_COUNTS* counts, uint64_t add_thread_time, uint64_t now);
```

`wait_for_next_event` is called with the mutex held. It waits until the next timer is due, a worker thread may have to be added, a worker thread may have to be retired, or the manager thread is woken.

**SRS_EXECUTION_ENGINE_LINUX_42_169: [** `wait_for_next_event` shall record that the manager thread is watching the ready queue if `add_thread_time` is not `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_170: [** `wait_for_next_event` shall get the time to retire a worker thread by calling `thread_count_policy_get_retire_thread_time` with `counts` and `now`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_171: [** `wait_for_next_event` shall get the first timer by calling `min_heap_peek`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_172: [** `wait_for_next_event` shall compute the deadline as the earliest of `add_thread_time`, the time to retire a worker thread and the due time of the first timer, if any. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_173: [** If the deadline is `THREAD_COUNT_POLICY_NEVER`, `wait_for_next_event` shall wait by calling `pthread_cond_wait`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_174: [** If the deadline is not `THREAD_COUNT_POLICY_NEVER`, `wait_for_next_event` shall wait until the deadline by calling `pthread_cond_timedwait`. **]**
