# `worker_pool_linux` requirements

## Overview

`worker_pool_linux` runs the worker threads of `execution_engine_linux`. It owns the threads and decides nothing about work: each worker thread repeatedly calls a `run_function` supplied by the caller, and becomes idle when `run_function` has no work to run.

The pool has no lock of its own. It uses the caller's mutex (`lock`), so the caller's state and the pool state change together. Except for `worker_pool_create` and `worker_pool_destroy`, the caller holds `lock` when calling the pool, and `run_function` is called with `lock` held. `run_function` can release `lock` while it runs work, and acquires it again before returning.

`worker_pool_add_worker`, `worker_pool_retire_oldest_idle_worker`, `worker_pool_join_retired_workers` and `worker_pool_destroy` must not be called concurrently: `worker_pool_add_worker` releases `lock` while it creates a thread, and the new worker is linked into the pool only after `lock` is acquired again. `execution_engine_linux` calls them from one thread at a time: `execution_engine_create` before the manager thread starts, then the manager thread, then the thread that tears down the execution engine.

- Idle worker threads are kept in a LIFO list: the most recently idle worker thread is woken first, so the same few threads stay warm, and the worker threads that are idle the longest are the ones retired.
- `worker_pool_wake_idle_worker` wakes at most one worker thread at a time: while a woken worker thread has not started running, it does not wake another one. When `run_function` leaves work for other worker threads, it must call `worker_pool_wake_idle_worker`, so woken worker threads wake each other one at a time. This avoids waking many threads for one work object.
- Each worker thread waits on its own condition variable, so waking one worker thread does not wake the others.
- A worker is in one state at a time: running, idle (in the list of idle workers), woken (signaled, not yet running) or retired (it exits).
- A retired worker thread exits, and the caller joins it later by calling `worker_pool_join_retired_workers` without holding up the other worker threads.

`WORKER_POOL_RUN_FUNCTION` is called on a worker thread with `lock` held. It returns `true` if it ran work and `false` if there was no work to run.

`WORKER_POOL_COUNTS` holds:
- `thread_count`: the number of worker threads, retired worker threads excluded.
- `starting_thread_count`: how many of them were created and did not start calling `run_function` yet.
- `idle_thread_count`: how many of them are idle.
- `oldest_idle_since_ms`: when the worker thread idle the longest became idle, 0 when no worker thread is idle. Times are in milliseconds as returned by `timer_global_get_elapsed_ms`.

## Exposed API

```c
typedef struct WORKER_POOL_TAG* WORKER_POOL_HANDLE;

typedef bool (*WORKER_POOL_RUN_FUNCTION)(void* context);

#define WORKER_POOL_WAKE_RESULT_VALUES \
    WORKER_POOL_WAKE_RESULT_WOKEN, \
    WORKER_POOL_WAKE_RESULT_ALREADY_WAKING, \
    WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER, \
    WORKER_POOL_WAKE_RESULT_INVALID_ARGS

MU_DEFINE_ENUM(WORKER_POOL_WAKE_RESULT, WORKER_POOL_WAKE_RESULT_VALUES);

typedef struct WORKER_POOL_COUNTS_TAG
{
    uint32_t thread_count;
    uint32_t starting_thread_count;
    uint32_t idle_thread_count;
    uint64_t oldest_idle_since_ms;
} WORKER_POOL_COUNTS;

MOCKABLE_FUNCTION(, WORKER_POOL_HANDLE, worker_pool_create, pthread_mutex_t*, lock, WORKER_POOL_RUN_FUNCTION, run_function, void*, run_function_context);
MOCKABLE_FUNCTION(, void, worker_pool_destroy, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, int, worker_pool_add_worker, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, WORKER_POOL_WAKE_RESULT, worker_pool_wake_idle_worker, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, void, worker_pool_get_counts, WORKER_POOL_HANDLE, worker_pool, WORKER_POOL_COUNTS*, counts);
MOCKABLE_FUNCTION(, void, worker_pool_retire_oldest_idle_worker, WORKER_POOL_HANDLE, worker_pool);
MOCKABLE_FUNCTION(, void, worker_pool_join_retired_workers, WORKER_POOL_HANDLE, worker_pool);
```

### worker_pool_create

```c
MOCKABLE_FUNCTION(, WORKER_POOL_HANDLE, worker_pool_create, pthread_mutex_t*, lock, WORKER_POOL_RUN_FUNCTION, run_function, void*, run_function_context);
```

`worker_pool_create` creates a pool with no worker threads.

**SRS_WORKER_POOL_LINUX_42_001: [** If `lock` is `NULL`, `worker_pool_create` shall fail and return `NULL`. **]**

**SRS_WORKER_POOL_LINUX_42_002: [** If `run_function` is `NULL`, `worker_pool_create` shall fail and return `NULL`. **]**

**SRS_WORKER_POOL_LINUX_42_003: [** `worker_pool_create` shall allocate a new pool. **]**

**SRS_WORKER_POOL_LINUX_42_004: [** `worker_pool_create` shall store `lock`, `run_function` and `run_function_context` in the pool. **]**

**SRS_WORKER_POOL_LINUX_42_005: [** `worker_pool_create` shall initialize the list of workers, the list of idle workers and the list of retired workers by calling `DList_InitializeListHead`. **]**

**SRS_WORKER_POOL_LINUX_42_006: [** `worker_pool_create` shall set the thread count, the starting thread count, the idle thread count and the signaled thread count to 0. **]**

**SRS_WORKER_POOL_LINUX_42_007: [** `worker_pool_create` shall mark the pool as not stopped. **]**

**SRS_WORKER_POOL_LINUX_42_008: [** `worker_pool_create` shall succeed and return the pool. **]**

**SRS_WORKER_POOL_LINUX_42_009: [** If any error occurs, `worker_pool_create` shall fail and return `NULL`. **]**

### worker_pool_destroy

```c
MOCKABLE_FUNCTION(, void, worker_pool_destroy, WORKER_POOL_HANDLE, worker_pool);
```

`worker_pool_destroy` stops the worker threads once they have no more work to run, waits for them to exit and frees the pool, without `lock` held by the caller.

**SRS_WORKER_POOL_LINUX_42_010: [** If `worker_pool` is `NULL`, `worker_pool_destroy` shall return. **]**

**SRS_WORKER_POOL_LINUX_42_011: [** `worker_pool_destroy` shall acquire `lock` by calling `pthread_mutex_lock`. **]**

**SRS_WORKER_POOL_LINUX_42_012: [** `worker_pool_destroy` shall mark the pool as stopped. **]**

**SRS_WORKER_POOL_LINUX_42_013: [** For each idle worker, `worker_pool_destroy` shall wake it by calling `pthread_cond_signal` on its condition variable. **]**

**SRS_WORKER_POOL_LINUX_42_014: [** `worker_pool_destroy` shall release `lock` by calling `pthread_mutex_unlock`. **]**

**SRS_WORKER_POOL_LINUX_42_015: [** For each worker in the list of workers: **]**

- **SRS_WORKER_POOL_LINUX_42_082: [** `worker_pool_destroy` shall remove the worker from the list of workers by calling `DList_RemoveHeadList`. **]**

- **SRS_WORKER_POOL_LINUX_42_083: [** `worker_pool_destroy` shall destroy the worker. **]**

**SRS_WORKER_POOL_LINUX_42_016: [** For each worker in the list of retired workers: **]**

- **SRS_WORKER_POOL_LINUX_42_084: [** `worker_pool_destroy` shall remove the worker from the list of retired workers by calling `DList_RemoveHeadList`. **]**

- **SRS_WORKER_POOL_LINUX_42_085: [** `worker_pool_destroy` shall destroy the worker. **]**

**SRS_WORKER_POOL_LINUX_42_017: [** `worker_pool_destroy` shall free the pool. **]**

### worker_pool_add_worker

```c
MOCKABLE_FUNCTION(, int, worker_pool_add_worker, WORKER_POOL_HANDLE, worker_pool);
```

`worker_pool_add_worker` starts a new worker thread, releasing `lock` while it creates the thread.

**SRS_WORKER_POOL_LINUX_42_018: [** If `worker_pool` is `NULL`, `worker_pool_add_worker` shall fail and return a non-zero value. **]**

**SRS_WORKER_POOL_LINUX_42_019: [** `worker_pool_add_worker` shall increment the thread count. **]**

**SRS_WORKER_POOL_LINUX_42_020: [** `worker_pool_add_worker` shall increment the starting thread count. **]**

**SRS_WORKER_POOL_LINUX_42_021: [** `worker_pool_add_worker` shall release `lock` by calling `pthread_mutex_unlock`. **]**

**SRS_WORKER_POOL_LINUX_42_022: [** `worker_pool_add_worker` shall create a worker. **]**

**SRS_WORKER_POOL_LINUX_42_023: [** `worker_pool_add_worker` shall acquire `lock` by calling `pthread_mutex_lock`. **]**

**SRS_WORKER_POOL_LINUX_42_024: [** If creating the worker fails: **]**

- **SRS_WORKER_POOL_LINUX_42_025: [** `worker_pool_add_worker` shall decrement the thread count. **]**

- **SRS_WORKER_POOL_LINUX_42_026: [** `worker_pool_add_worker` shall decrement the starting thread count. **]**

- **SRS_WORKER_POOL_LINUX_42_027: [** `worker_pool_add_worker` shall fail and return a non-zero value. **]**

**SRS_WORKER_POOL_LINUX_42_028: [** If creating the worker succeeds: **]**

- **SRS_WORKER_POOL_LINUX_42_029: [** `worker_pool_add_worker` shall insert the worker in the list of workers by calling `DList_InsertTailList`. **]**

- **SRS_WORKER_POOL_LINUX_42_030: [** `worker_pool_add_worker` shall succeed and return 0. **]**

### internal_worker_pool_create_worker

```c
static WORKER* internal_worker_pool_create_worker(WORKER_POOL* worker_pool);
```

`internal_worker_pool_create_worker` creates a worker and starts its thread, without `lock` held.

**SRS_WORKER_POOL_LINUX_42_031: [** `internal_worker_pool_create_worker` shall allocate a worker. **]**

**SRS_WORKER_POOL_LINUX_42_086: [** `internal_worker_pool_create_worker` shall store `worker_pool` in the worker. **]**

**SRS_WORKER_POOL_LINUX_42_032: [** `internal_worker_pool_create_worker` shall set the state of the worker to running. **]**

**SRS_WORKER_POOL_LINUX_42_033: [** `internal_worker_pool_create_worker` shall initialize the condition variable of the worker by calling `pthread_cond_init`. **]**

**SRS_WORKER_POOL_LINUX_42_034: [** `internal_worker_pool_create_worker` shall start the worker thread by calling `ThreadAPI_Create` with `internal_worker_pool_worker_thread_func` and the worker. **]**

**SRS_WORKER_POOL_LINUX_42_035: [** `internal_worker_pool_create_worker` shall return the worker. **]**

**SRS_WORKER_POOL_LINUX_42_036: [** If any error occurs, `internal_worker_pool_create_worker` shall free the resources it allocated and return `NULL`. **]**

### internal_worker_pool_destroy_worker

```c
static void internal_worker_pool_destroy_worker(WORKER* worker);
```

`internal_worker_pool_destroy_worker` waits for the worker thread to exit and frees the worker.

**SRS_WORKER_POOL_LINUX_42_037: [** `internal_worker_pool_destroy_worker` shall wait for the worker thread to exit by calling `ThreadAPI_Join`. **]**

**SRS_WORKER_POOL_LINUX_42_087: [** If `ThreadAPI_Join` fails, `internal_worker_pool_destroy_worker` shall return without freeing the worker, which the worker thread can still use. **]**

**SRS_WORKER_POOL_LINUX_42_038: [** `internal_worker_pool_destroy_worker` shall destroy the condition variable of the worker by calling `pthread_cond_destroy`. **]**

**SRS_WORKER_POOL_LINUX_42_039: [** `internal_worker_pool_destroy_worker` shall free the worker. **]**

### internal_worker_pool_worker_thread_func

```c
static int internal_worker_pool_worker_thread_func(void* arg);
```

`internal_worker_pool_worker_thread_func` is the function of the worker threads, with the worker as `arg`.

**SRS_WORKER_POOL_LINUX_42_040: [** `internal_worker_pool_worker_thread_func` shall acquire `lock` by calling `pthread_mutex_lock`. **]**

**SRS_WORKER_POOL_LINUX_42_041: [** `internal_worker_pool_worker_thread_func` shall decrement the starting thread count. **]**

**SRS_WORKER_POOL_LINUX_42_042: [** Until the worker is retired or the worker stops: **]**

- **SRS_WORKER_POOL_LINUX_42_043: [** `internal_worker_pool_worker_thread_func` shall call `run_function` with `run_function_context`. **]**

- **SRS_WORKER_POOL_LINUX_42_044: [** If `run_function` returns `false`: **]**

  - **SRS_WORKER_POOL_LINUX_42_045: [** If the pool is stopped, `internal_worker_pool_worker_thread_func` shall stop. **]**

  - **SRS_WORKER_POOL_LINUX_42_046: [** If the pool is not stopped, `internal_worker_pool_worker_thread_func` shall wait while the worker is idle. **]**

**SRS_WORKER_POOL_LINUX_42_047: [** `internal_worker_pool_worker_thread_func` shall release `lock` by calling `pthread_mutex_unlock`. **]**

**SRS_WORKER_POOL_LINUX_42_048: [** `internal_worker_pool_worker_thread_func` shall return 0. **]**

### internal_worker_pool_wait_while_idle

```c
static void internal_worker_pool_wait_while_idle(WORKER_POOL* worker_pool, WORKER* worker);
```

`internal_worker_pool_wait_while_idle` makes the worker idle until it is woken, retired or the pool is stopped, on the worker thread with `lock` held.

**SRS_WORKER_POOL_LINUX_42_049: [** `internal_worker_pool_wait_while_idle` shall set the state of the worker to idle. **]**

**SRS_WORKER_POOL_LINUX_42_050: [** `internal_worker_pool_wait_while_idle` shall record the current time obtained by calling `timer_global_get_elapsed_ms` as the time the worker became idle. **]**

**SRS_WORKER_POOL_LINUX_42_051: [** `internal_worker_pool_wait_while_idle` shall insert the worker at the head of the list of idle workers by calling `DList_InsertHeadList`. **]**

**SRS_WORKER_POOL_LINUX_42_088: [** `internal_worker_pool_wait_while_idle` shall increment the idle thread count. **]**

**SRS_WORKER_POOL_LINUX_42_052: [** `internal_worker_pool_wait_while_idle` shall wait by calling `pthread_cond_wait` on the condition variable of the worker while the worker is idle and the pool is not stopped. **]**

**SRS_WORKER_POOL_LINUX_42_053: [** If the worker was woken: **]**

- **SRS_WORKER_POOL_LINUX_42_089: [** `internal_worker_pool_wait_while_idle` shall decrement the signaled thread count. **]**

- **SRS_WORKER_POOL_LINUX_42_090: [** `internal_worker_pool_wait_while_idle` shall set the state of the worker to running. **]**

**SRS_WORKER_POOL_LINUX_42_054: [** If the worker is still idle: **]**

- **SRS_WORKER_POOL_LINUX_42_091: [** `internal_worker_pool_wait_while_idle` shall remove the worker from the list of idle workers by calling `DList_RemoveEntryList`. **]**

- **SRS_WORKER_POOL_LINUX_42_092: [** `internal_worker_pool_wait_while_idle` shall decrement the idle thread count. **]**

- **SRS_WORKER_POOL_LINUX_42_093: [** `internal_worker_pool_wait_while_idle` shall set the state of the worker to running. **]**

### worker_pool_wake_idle_worker

```c
MOCKABLE_FUNCTION(, WORKER_POOL_WAKE_RESULT, worker_pool_wake_idle_worker, WORKER_POOL_HANDLE, worker_pool);
```

`worker_pool_wake_idle_worker` wakes the most recently idle worker, unless a woken worker has not started running yet.

**SRS_WORKER_POOL_LINUX_42_055: [** If `worker_pool` is `NULL`, `worker_pool_wake_idle_worker` shall return `WORKER_POOL_WAKE_RESULT_INVALID_ARGS`. **]**

**SRS_WORKER_POOL_LINUX_42_056: [** If no worker is idle, `worker_pool_wake_idle_worker` shall return `WORKER_POOL_WAKE_RESULT_NO_IDLE_WORKER`. **]**

**SRS_WORKER_POOL_LINUX_42_057: [** If the signaled thread count is not 0, `worker_pool_wake_idle_worker` shall return `WORKER_POOL_WAKE_RESULT_ALREADY_WAKING`. **]**

**SRS_WORKER_POOL_LINUX_42_058: [** `worker_pool_wake_idle_worker` shall remove the worker at the head of the list of idle workers by calling `DList_RemoveHeadList`. **]**

**SRS_WORKER_POOL_LINUX_42_094: [** `worker_pool_wake_idle_worker` shall decrement the idle thread count. **]**

**SRS_WORKER_POOL_LINUX_42_059: [** `worker_pool_wake_idle_worker` shall set the state of the worker to woken. **]**

**SRS_WORKER_POOL_LINUX_42_060: [** `worker_pool_wake_idle_worker` shall increment the signaled thread count. **]**

**SRS_WORKER_POOL_LINUX_42_061: [** `worker_pool_wake_idle_worker` shall wake the worker by calling `pthread_cond_signal` on its condition variable. **]**

**SRS_WORKER_POOL_LINUX_42_062: [** `worker_pool_wake_idle_worker` shall return `WORKER_POOL_WAKE_RESULT_WOKEN`. **]**

### worker_pool_get_counts

```c
MOCKABLE_FUNCTION(, void, worker_pool_get_counts, WORKER_POOL_HANDLE, worker_pool, WORKER_POOL_COUNTS*, counts);
```

`worker_pool_get_counts` reports the thread counts of the pool.

**SRS_WORKER_POOL_LINUX_42_063: [** If `worker_pool` is `NULL`, `worker_pool_get_counts` shall return. **]**

**SRS_WORKER_POOL_LINUX_42_064: [** If `counts` is `NULL`, `worker_pool_get_counts` shall return. **]**

**SRS_WORKER_POOL_LINUX_42_065: [** `worker_pool_get_counts` shall store the thread count, the starting thread count and the idle thread count in `counts`. **]**

**SRS_WORKER_POOL_LINUX_42_066: [** If no worker is idle, `worker_pool_get_counts` shall set `oldest_idle_since_ms` to 0. **]**

**SRS_WORKER_POOL_LINUX_42_067: [** If a worker is idle, `worker_pool_get_counts` shall set `oldest_idle_since_ms` to the time the worker at the tail of the list of idle workers became idle. **]**

### worker_pool_retire_oldest_idle_worker

```c
MOCKABLE_FUNCTION(, void, worker_pool_retire_oldest_idle_worker, WORKER_POOL_HANDLE, worker_pool);
```

`worker_pool_retire_oldest_idle_worker` makes the worker idle the longest exit.

**SRS_WORKER_POOL_LINUX_42_068: [** If `worker_pool` is `NULL`, `worker_pool_retire_oldest_idle_worker` shall return. **]**

**SRS_WORKER_POOL_LINUX_42_069: [** If no worker is idle, `worker_pool_retire_oldest_idle_worker` shall return. **]**

**SRS_WORKER_POOL_LINUX_42_070: [** `worker_pool_retire_oldest_idle_worker` shall remove the worker at the tail of the list of idle workers by calling `DList_RemoveTailList`. **]**

**SRS_WORKER_POOL_LINUX_42_095: [** `worker_pool_retire_oldest_idle_worker` shall decrement the idle thread count. **]**

**SRS_WORKER_POOL_LINUX_42_071: [** `worker_pool_retire_oldest_idle_worker` shall set the state of the worker to retired. **]**

**SRS_WORKER_POOL_LINUX_42_072: [** `worker_pool_retire_oldest_idle_worker` shall wake the worker by calling `pthread_cond_signal` on its condition variable. **]**

**SRS_WORKER_POOL_LINUX_42_073: [** `worker_pool_retire_oldest_idle_worker` shall decrement the thread count. **]**

**SRS_WORKER_POOL_LINUX_42_074: [** `worker_pool_retire_oldest_idle_worker` shall remove the worker from the list of workers by calling `DList_RemoveEntryList`. **]**

**SRS_WORKER_POOL_LINUX_42_075: [** `worker_pool_retire_oldest_idle_worker` shall insert the worker in the list of retired workers by calling `DList_InsertTailList`. **]**

### worker_pool_join_retired_workers

```c
MOCKABLE_FUNCTION(, void, worker_pool_join_retired_workers, WORKER_POOL_HANDLE, worker_pool);
```

`worker_pool_join_retired_workers` waits for the retired workers to exit and frees them, releasing `lock` while it waits.

**SRS_WORKER_POOL_LINUX_42_076: [** If `worker_pool` is `NULL`, `worker_pool_join_retired_workers` shall return. **]**

**SRS_WORKER_POOL_LINUX_42_077: [** For each worker in the list of retired workers: **]**

- **SRS_WORKER_POOL_LINUX_42_078: [** `worker_pool_join_retired_workers` shall remove the worker from the list of retired workers by calling `DList_RemoveHeadList`. **]**

- **SRS_WORKER_POOL_LINUX_42_079: [** `worker_pool_join_retired_workers` shall release `lock` by calling `pthread_mutex_unlock`. **]**

- **SRS_WORKER_POOL_LINUX_42_080: [** `worker_pool_join_retired_workers` shall destroy the worker. **]**

- **SRS_WORKER_POOL_LINUX_42_081: [** `worker_pool_join_retired_workers` shall acquire `lock` by calling `pthread_mutex_lock`. **]**
