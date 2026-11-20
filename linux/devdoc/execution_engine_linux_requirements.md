# `execution_engine_linux` requirements

## Overview

`execution_engine_linux` is the Linux implementation of the `execution_engine` interface.

An execution engine is the shared thread pool used by every `threadpool` created on it, the Linux equivalent of a Win32 `PTP_POOL`. It owns the worker threads, the ready queue and the timers, and it exposes a small Linux-specific API (modeled on the Win32 `CreateThreadpoolWork` / `SubmitThreadpoolWork` / `SetThreadpoolTimer` / `WaitForThreadpoolWorkCallbacks` / `CloseThreadpoolWork` family) that `threadpool_linux` maps onto.

## Design

### Work objects

A work object (`EXECUTION_ENGINE_LINUX_WORK_HANDLE`) is created once (this can fail) and then submitted any number of times. Submitting never allocates and cannot fail: it increments the work object's pending count and, when the count goes from 0 to 1, links the work object at the tail of the engine's ready queue. A worker that takes a work object from the queue decrements its pending count and, if invocations are still pending, links it back at the tail before running the callback, so several invocations of the same work object can run concurrently, as with Win32.

Each work object tracks its pending and running invocations. `execution_engine_linux_wait_for_work` waits until both are zero, optionally dropping the invocations that have not started. `execution_engine_linux_close_work` frees the work object, immediately if it has no outstanding invocations, otherwise after the last one completes, so it can be called from the work object's own callback.

A callback can disassociate its thread from its work object (`execution_engine_linux_disassociate_current_thread_from_work`, like Win32 `DisassociateCurrentThreadFromCallback`): its invocation then no longer counts as running, so the callback can wait for or close its own work object, and threads already waiting for the work object stop waiting for it. A thread-local variable records the work object whose callback runs on the current thread, and the worker does not touch the work object again once its thread was disassociated from it.

### Timers

A timer is a work object that the engine submits when it expires. The engine keeps the armed timers in a binary min-heap ordered by due time. The heap capacity is grown when a timer is created (which can fail), so that it always holds every timer of the engine; arming, re-arming and cancelling a timer therefore never allocate and cannot fail. Due times use `CLOCK_MONOTONIC`. A start delay of 0 expires immediately.

### Threads

The engine has one manager thread and between `min_thread_count` and the maximum number of worker threads. The maximum is `max_thread_count`, or 500 (the Win32 default) when `max_thread_count` is 0.

Idle workers wait on their own condition variable in a LIFO list, so the most recently idle (cache-hot) worker is woken first. At most one worker is woken and not yet running at any time; when it starts running and work is still queued, it wakes the next idle worker. This keeps a fast producer from paying a thread wake for every submitted item. All engine state is guarded by one mutex.

The manager thread:
- fires due timers,
- creates worker threads,
- retires worker threads above `min_thread_count` that have been idle for 60 seconds,
- joins retired worker threads.

Worker threads are never created on the submit path, so submitting stays infallible and a failed thread creation is retried 100 ms later.

`min_thread_count` worker threads are created with the engine. When work is queued, no worker is idle and the ready queue has made no progress (no work object was taken from it) for 50 ms, the manager creates another worker, at most one per 50 ms. This covers callbacks that block (including callbacks that wait for other callbacks), without creating threads for CPU-bound work that keeps the existing workers busy: one mutex guards the engine, so for short callbacks more threads mean more contention, not more throughput.

Linux gives no notification when a thread blocks, which is how Win32 injects threads immediately. The 50 ms starvation threshold is the latency cost of that.

To keep the submit path cheap, the manager thread is only woken when work is queued with no idle worker and the manager is not already waiting to check the ready queue for progress. Workers becoming idle do not wake it; while there are more workers than `min_thread_count` it checks for workers to retire at least every 60 seconds.

### Lifetime

Work objects must be closed before the last reference to the execution engine is released. The last reference must not be released from a callback running on the same execution engine, since the engine joins its own threads when it is destroyed.

## Exposed API

`execution_engine_linux` implements the `execution_engine` API and additionally exposes the following API:

```c
typedef struct EXECUTION_ENGINE_LINUX_WORK_TAG* EXECUTION_ENGINE_LINUX_WORK_HANDLE;
typedef void (*EXECUTION_ENGINE_LINUX_WORK_FUNCTION)(void* context);

MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);

MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_work, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_timer, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
MOCKABLE_FUNCTION(, void, execution_engine_linux_submit_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
MOCKABLE_FUNCTION(, void, execution_engine_linux_set_timer, EXECUTION_ENGINE_LINUX_WORK_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
MOCKABLE_FUNCTION(, void, execution_engine_linux_cancel_timer, EXECUTION_ENGINE_LINUX_WORK_HANDLE, timer);
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work, bool, cancel_pending);
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

### execution_engine_create

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_HANDLE, execution_engine_create, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_parameters);
```

`execution_engine_create` creates an execution engine and starts its threads.

**SRS_EXECUTION_ENGINE_LINUX_07_001: [** `execution_engine_create` shall allocate a new execution engine and on success shall return a non-`NULL` handle. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_002: [** If `execution_engine_parameters` is `NULL`, `execution_engine_create` shall use the default `DEFAULT_MIN_THREAD_COUNT` and `DEFAULT_MAX_THREAD_COUNT` as parameters. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_003: [** `execution_engine_create` shall set the minimum number of threads to the `min_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_004: [** `execution_engine_create` shall set the maximum number of threads to the `max_thread_count` field of `execution_engine_parameters`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_005: [** If `max_thread_count` is non-zero and less than `min_thread_count`, `execution_engine_create` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_001: [** If `max_thread_count` is 0, `execution_engine_create` shall use 500 as the maximum number of worker threads, or `min_thread_count` if it is larger. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_003: [** `execution_engine_create` shall initialize the mutex guarding the execution engine state by calling `pthread_mutex_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_004: [** `execution_engine_create` shall initialize the condition variable used to wake the manager thread with an attribute that uses `CLOCK_MONOTONIC` by calling `pthread_condattr_init`, `pthread_condattr_setclock`, `pthread_cond_init` and `pthread_condattr_destroy`, and the condition variable used to signal completed work by calling `pthread_cond_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_005: [** `execution_engine_create` shall create `min_thread_count` worker threads. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_006: [** `execution_engine_create` shall create the manager thread by calling `ThreadAPI_Create` with `manager_thread_func`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_006: [** If any error occurs, `execution_engine_create` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_007: [** If any error occurs after worker threads were created, `execution_engine_create` shall signal them to stop and join them. **]**

### create_worker (internal)

```c
static int create_worker(EXECUTION_ENGINE* execution_engine, WORKER** worker_out);
```

`create_worker` creates one worker thread.

**SRS_EXECUTION_ENGINE_LINUX_42_008: [** `create_worker` shall allocate a worker. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_009: [** `create_worker` shall initialize the condition variable used to wake the worker by calling `pthread_cond_init`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_010: [** `create_worker` shall start the worker thread by calling `ThreadAPI_Create` with `worker_thread_func`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_011: [** If any error occurs, `create_worker` shall free the resources it allocated and fail. **]**

### execution_engine_dec_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_dec_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_dec_ref` decrements the ref count and, when it reaches 0, stops the threads and frees the execution engine.

**SRS_EXECUTION_ENGINE_LINUX_07_007: [** If `execution_engine` is `NULL`, `execution_engine_dec_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_008: [** Otherwise `execution_engine_dec_ref` shall decrement the refcount. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_012: [** If the refcount reaches 0 on a worker thread of `execution_engine`, `execution_engine_dec_ref` shall terminate the process by calling `LogCriticalAndTerminate`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_013: [** Otherwise, if the refcount reaches 0, `execution_engine_dec_ref` shall set the stop flag and signal the manager thread and every idle worker. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_014: [** `execution_engine_dec_ref` shall join the manager thread and all the worker threads. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_009: [** If the refcount is zero `execution_engine_dec_ref` shall free the memory for `EXECUTION_ENGINE`. **]**

### execution_engine_inc_ref

```c
MOCKABLE_FUNCTION(, void, execution_engine_inc_ref, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_inc_ref` increments the ref count on the `execution_engine`.

**SRS_EXECUTION_ENGINE_LINUX_07_010: [** If `execution_engine` is `NULL` then `execution_engine_inc_ref` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_011: [** Otherwise `execution_engine_inc_ref` shall increment the reference count for `execution_engine`. **]**

### execution_engine_linux_get_parameters

```c
MOCKABLE_FUNCTION(, const EXECUTION_ENGINE_PARAMETERS*, execution_engine_linux_get_parameters, EXECUTION_ENGINE_HANDLE, execution_engine);
```

`execution_engine_linux_get_parameters` returns the `EXECUTION_ENGINE_PARAMETERS` of the execution engine.

**SRS_EXECUTION_ENGINE_LINUX_07_012: [** If `execution_engine` is `NULL`, `execution_engine_linux_get_parameters` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_07_013: [** Otherwise, `execution_engine_linux_get_parameters` shall return the parameters in `EXECUTION_ENGINE`. **]**

### execution_engine_linux_create_work

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_work, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
```

`execution_engine_linux_create_work` creates a work object that can be submitted many times.

**SRS_EXECUTION_ENGINE_LINUX_42_015: [** If `execution_engine` is `NULL`, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_016: [** If `work_function` is `NULL`, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_017: [** `execution_engine_linux_create_work` shall allocate a work object, store `work_function` and `work_function_context` in it, and return it. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_018: [** If any error occurs, `execution_engine_linux_create_work` shall fail and return `NULL`. **]**

### execution_engine_linux_create_timer

```c
MOCKABLE_FUNCTION(, EXECUTION_ENGINE_LINUX_WORK_HANDLE, execution_engine_linux_create_timer, EXECUTION_ENGINE_HANDLE, execution_engine, EXECUTION_ENGINE_LINUX_WORK_FUNCTION, work_function, void*, work_function_context);
```

`execution_engine_linux_create_timer` creates a timer: a work object that the execution engine submits when it expires. The timer is not armed.

**SRS_EXECUTION_ENGINE_LINUX_42_019: [** If `execution_engine` is `NULL`, `execution_engine_linux_create_timer` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_020: [** If `work_function` is `NULL`, `execution_engine_linux_create_timer` shall fail and return `NULL`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_021: [** `execution_engine_linux_create_timer` shall allocate a work object marked as a timer and store `work_function` and `work_function_context` in it. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_022: [** `execution_engine_linux_create_timer` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_023: [** If the timer heap is full, `execution_engine_linux_create_timer` shall double its capacity (16 if it is empty) by calling `realloc_2`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_024: [** `execution_engine_linux_create_timer` shall increment the timer count, release the mutex and return the timer. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_025: [** If any error occurs, `execution_engine_linux_create_timer` shall release the mutex if acquired, free the work object, fail and return `NULL`. **]**

### execution_engine_linux_submit_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_submit_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_submit_work` queues one invocation of the work object's callback. It does not allocate.

**SRS_EXECUTION_ENGINE_LINUX_42_026: [** If `work` is `NULL`, `execution_engine_linux_submit_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_027: [** `execution_engine_linux_submit_work` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_028: [** If the pending count of `work` is `UINT32_MAX`, `execution_engine_linux_submit_work` shall log an error and not queue another invocation. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_029: [** Otherwise `execution_engine_linux_submit_work` shall increment the pending count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_030: [** If the pending count became 1, `execution_engine_linux_submit_work` shall link `work` at the tail of the ready queue and dispatch it. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_031: [** `execution_engine_linux_submit_work` shall release the mutex. **]**

### enqueue_and_dispatch (internal)

```c
static void enqueue_and_dispatch(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work);
```

`enqueue_and_dispatch` is called with the mutex held whenever a work object is linked into the ready queue.

**SRS_EXECUTION_ENGINE_LINUX_42_032: [** If the ready queue was empty, `enqueue_and_dispatch` shall record the current time as the last progress time. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_033: [** `enqueue_and_dispatch` shall link `work` at the tail of the ready queue. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_034: [** If a worker is idle and no other worker was woken and has not started running yet, `enqueue_and_dispatch` shall remove the most recently idle worker from the idle list, mark it signaled and wake it by calling `pthread_cond_signal`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_082: [** If a worker is idle and another worker was woken and has not started running yet, `enqueue_and_dispatch` shall not wake another worker. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_035: [** Otherwise, if the thread count is below the maximum and the manager thread is not already waiting to check the ready queue for progress, `enqueue_and_dispatch` shall wake the manager thread by calling `pthread_cond_signal`. **]**

### execution_engine_linux_set_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_set_timer, EXECUTION_ENGINE_LINUX_WORK_HANDLE, timer, uint32_t, start_delay_ms, uint32_t, timer_period_ms);
```

`execution_engine_linux_set_timer` arms or re-arms a timer. It does not allocate. Invocations already queued are not affected.

**SRS_EXECUTION_ENGINE_LINUX_42_036: [** If `timer` is `NULL`, `execution_engine_linux_set_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_037: [** If `timer` is not a timer, `execution_engine_linux_set_timer` shall log an error and return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_038: [** `execution_engine_linux_set_timer` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_039: [** If `timer` is armed, `execution_engine_linux_set_timer` shall remove it from the timer heap. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_040: [** `execution_engine_linux_set_timer` shall set the due time of `timer` to the current `CLOCK_MONOTONIC` time obtained by calling `clock_gettime` plus `start_delay_ms`, store `timer_period_ms` and insert `timer` in the timer heap. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_041: [** `execution_engine_linux_set_timer` shall wake the manager thread by calling `pthread_cond_signal` and release the mutex. **]**

### execution_engine_linux_cancel_timer

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_cancel_timer, EXECUTION_ENGINE_LINUX_WORK_HANDLE, timer);
```

`execution_engine_linux_cancel_timer` disarms a timer. Invocations already queued or running are not affected; use `execution_engine_linux_wait_for_work` to drop or wait for them.

**SRS_EXECUTION_ENGINE_LINUX_42_042: [** If `timer` is `NULL`, `execution_engine_linux_cancel_timer` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_043: [** If `timer` is not a timer, `execution_engine_linux_cancel_timer` shall log an error and return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_044: [** `execution_engine_linux_cancel_timer` shall acquire the execution engine mutex, remove `timer` from the timer heap if it is armed and release the mutex. **]**

### execution_engine_linux_wait_for_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_wait_for_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work, bool, cancel_pending);
```

`execution_engine_linux_wait_for_work` waits until the work object has no pending and no running invocations. It must not be called from the work object's own callback.

**SRS_EXECUTION_ENGINE_LINUX_42_045: [** If `work` is `NULL`, `execution_engine_linux_wait_for_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_046: [** `execution_engine_linux_wait_for_work` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_047: [** If `cancel_pending` is `true` and `work` has pending invocations, `execution_engine_linux_wait_for_work` shall unlink `work` from the ready queue and set its pending count to 0. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_048: [** `execution_engine_linux_wait_for_work` shall wait by calling `pthread_cond_wait` until `work` has no pending and no running invocations. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_049: [** `execution_engine_linux_wait_for_work` shall release the mutex. **]**

### execution_engine_linux_close_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_close_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_close_work` releases a work object. Invocations already queued still run. It may be called from the work object's own callback.

**SRS_EXECUTION_ENGINE_LINUX_42_050: [** If `work` is `NULL`, `execution_engine_linux_close_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_051: [** `execution_engine_linux_close_work` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_052: [** If `work` is an armed timer, `execution_engine_linux_close_work` shall remove it from the timer heap. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_053: [** If `work` has no pending and no running invocations, `execution_engine_linux_close_work` shall free it. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_054: [** Otherwise `execution_engine_linux_close_work` shall mark `work` as closed so that the worker running its last invocation frees it. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_055: [** `execution_engine_linux_close_work` shall release the mutex. **]**

### execution_engine_linux_disassociate_current_thread_from_work

```c
MOCKABLE_FUNCTION(, void, execution_engine_linux_disassociate_current_thread_from_work, EXECUTION_ENGINE_LINUX_WORK_HANDLE, work);
```

`execution_engine_linux_disassociate_current_thread_from_work` is the equivalent of Win32 `DisassociateCurrentThreadFromCallback`. Called from a callback of `work`, it ends that invocation early: `execution_engine_linux_wait_for_work` no longer waits for it, so the callback can wait for, or close, its own work object. The callback must not use `work` after closing it.

**SRS_EXECUTION_ENGINE_LINUX_42_084: [** If `work` is `NULL`, `execution_engine_linux_disassociate_current_thread_from_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_085: [** If the current thread is not running a callback of `work`, or was already disassociated from it, `execution_engine_linux_disassociate_current_thread_from_work` shall return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_086: [** Otherwise `execution_engine_linux_disassociate_current_thread_from_work` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_087: [** `execution_engine_linux_disassociate_current_thread_from_work` shall record that the current thread no longer runs a callback of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_088: [** `execution_engine_linux_disassociate_current_thread_from_work` shall end the invocation of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_089: [** `execution_engine_linux_disassociate_current_thread_from_work` shall release the mutex. **]**

### end_invocation (internal)

```c
static void end_invocation(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work);
```

`end_invocation` is called with the mutex held when an invocation of `work` completes or its thread is disassociated from it.

**SRS_EXECUTION_ENGINE_LINUX_42_065: [** `end_invocation` shall decrement the running count of `work`. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_066: [** If `work` has no pending and no running invocations and it is closed, `end_invocation` shall free it. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_067: [** If `work` has no pending and no running invocations, it is not closed and threads are waiting for it, `end_invocation` shall wake them by calling `pthread_cond_broadcast`. **]**

### free_work (internal)

```c
static void free_work(EXECUTION_ENGINE* execution_engine, EXECUTION_ENGINE_LINUX_WORK* work);
```

**SRS_EXECUTION_ENGINE_LINUX_42_056: [** If `work` is a timer, `free_work` shall decrement the timer count. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_057: [** `free_work` shall free `work`. **]**

### worker_thread_func (internal)

```c
static int worker_thread_func(void* arg);
```

`worker_thread_func` runs queued invocations.

**SRS_EXECUTION_ENGINE_LINUX_42_058: [** `worker_thread_func` shall record the execution engine as the current execution engine of the thread until it returns. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_059: [** `worker_thread_func` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_081: [** If the worker was retired, `worker_thread_func` shall release the mutex and return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_060: [** Otherwise, if the ready queue is not empty, `worker_thread_func` shall: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_061: [** unlink the work object at the head of the ready queue and decrement its pending count. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_062: [** If the work object still has pending invocations, link it back at the tail of the ready queue and dispatch it. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_083: [** If the ready queue is still not empty, a worker is idle and no other worker was woken and has not started running yet, wake the most recently idle worker. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_063: [** Increment the running count of the work object and record the current time as the last progress time. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_064: [** Record the work object as the work running on the current thread, release the mutex, call the work function with the work function context and acquire the mutex. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_090: [** If the current thread was disassociated from the work object while the work function ran, go on to the next work object without accessing the work object, which can be freed by then. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_091: [** Otherwise, clear the work running on the current thread and end the invocation of the work object. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_068: [** Otherwise, if the stop flag is set, `worker_thread_func` shall release the mutex and return. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_069: [** Otherwise `worker_thread_func` shall add the worker to the front of the idle list and wait by calling `pthread_cond_wait` until it is signaled, retired or the stop flag is set. **]**

### manager_thread_func (internal)

```c
static int manager_thread_func(void* arg);
```

`manager_thread_func` fires timers and creates, retires and joins worker threads.

**SRS_EXECUTION_ENGINE_LINUX_42_070: [** `manager_thread_func` shall acquire the execution engine mutex. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_071: [** Until the stop flag is set, `manager_thread_func` shall: **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_072: [** Get the current `CLOCK_MONOTONIC` time by calling `clock_gettime`. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_073: [** For each timer in the timer heap whose due time has passed, remove it from the heap, submit it and, if its period is not 0, insert it back with its due time advanced by the period (or the current time plus the period if that is not later than the current time). **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_074: [** While the thread count is above `min_thread_count` and the least recently idle worker has been idle for at least 60000 ms, remove it from the idle list, mark it retired, wake it by calling `pthread_cond_signal` and decrement the thread count. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_075: [** For each retired worker, release the mutex, join it by calling `ThreadAPI_Join`, free it and acquire the mutex. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_076: [** If the ready queue has more work objects than workers being started, no worker is idle, the thread count is below the maximum, the thread creation retry time has passed and no work object was taken from the ready queue for 50 ms, increment the thread count, release the mutex, create a worker and acquire the mutex. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_077: [** If creating the worker fails, decrement the thread count and set the thread creation retry time to 100 ms from now. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_078: [** If creating the worker succeeds, record the current time as the last progress time. **]**

- **SRS_EXECUTION_ENGINE_LINUX_42_079: [** Otherwise wait for the earliest of the next timer due time, the next time a worker may need to be created and, while the thread count is above `min_thread_count`, the next time an idle worker may need to be retired (at most 60000 ms) by calling `pthread_cond_timedwait`, or by calling `pthread_cond_wait` if there is no such time. **]**

**SRS_EXECUTION_ENGINE_LINUX_42_080: [** `manager_thread_func` shall release the mutex and return. **]**
