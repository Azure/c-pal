# robust_semaphore requirements

## Overview

`robust_semaphore` lets at most `max_count` holders, across all the processes on a machine, hold a unit of a named semaphore at the same time. Unlike a Windows semaphore, a unit held by a process that dies is given back.

A typical use is to limit how many instances of an expensive activity run at the same time on a machine, for example how many replicas are being built, across all the processes that run them.

`robust_semaphore` is only implemented for Windows.

## Design

### Why mutexes and not a Windows semaphore

The count of a Windows semaphore is not given back when a process that took it dies, so a crash would permanently lower the limit. A mutex is owned by the thread that acquired it: when that thread (or its process) ends without releasing it, the mutex is abandoned and the next waiter acquires it (`WAIT_ABANDONED`).

So a `robust_semaphore` named `name` with `max_count` units is made of `max_count` named mutexes `<name>_0` ... `<name>_<max_count - 1>`. Holding a unit means owning one of these mutexes.

### Parts

- **The robust semaphore object** (`ROBUST_SEMAPHORE_HANDLE`). Each user (for example each process or each component that needs a unit) creates its own. It opens the `max_count` mutexes once, at create. It supports one acquisition at a time: a new acquisition can be started once the previous one ended. An acquisition that got a unit ends when it is released (or when `max_hold_ms` expires); an acquisition that did not get a unit ends just before its `on_acquire_complete` is called (so the callback can start a new one). The worker thread releases the mutex of a released acquisition before it takes the next one.
- **The worker thread** (`robust_semaphore_worker_thread`). Because a mutex belongs to the thread that acquired it, the worker thread is the only thread that waits on and releases the mutexes. It waits for a unit, calls `on_acquire_complete`, holds the unit until it is released and releases it. The worker thread only exists while there is an acquisition, so an idle robust semaphore object has no thread:
  - `robust_semaphore_acquire_async` starts a worker thread when none is running (after waiting for the previous one, which already decided to exit, to end).
  - When its acquisition ended, the worker thread takes the next acquisition if one was started in the meantime (for example from `on_acquire_complete`, or after a release while the worker thread was still releasing the mutex). Otherwise it exits. The decision to exit is made under the lock, and the exiting worker thread does not take the lock again, so `robust_semaphore_acquire_async` can wait for it while holding the lock.
  - `robust_semaphore_destroy` waits for the worker thread to end, if one was started.
- **The events**, each with one job:
  - **cancel event** (manual reset): stop waiting for a unit.
  - **release event** (manual reset): stop holding the unit.

### States of an acquisition

| State | Entered when | Worker thread | Leaves when |
|---|---|---|---|
| IDLE | create, or the previous acquisition ended | takes the next acquisition if one was started, otherwise exits (there is no worker thread while the object is idle) | `robust_semaphore_acquire_async` (to WAITING, starting a worker thread if none is running) |
| WAITING | the worker thread picks up the acquisition | waits for the cancel event or any of the mutexes, up to `timeout_ms` | a unit is acquired (to HOLDING, `on_acquire_complete` called with `ROBUST_SEMAPHORE_ACQUIRE_OK`), or cancel/release/destroy, timeout or error (to IDLE, `on_acquire_complete` called with the result) |
| HOLDING | a mutex was acquired | calls `on_acquire_complete`, then waits for the release event, up to what is left of `max_hold_ms` | release/destroy, or `max_hold_ms` expired (the mutex is released early): the worker thread releases the mutex (to IDLE) |

`on_acquire_complete` is always called on the worker thread, never on the thread calling `robust_semaphore_acquire_async`. Because the worker thread may acquire a free unit right away, it can run before `robust_semaphore_acquire_async` returns, so the caller must have everything the callback uses in place before calling `robust_semaphore_acquire_async`. If `robust_semaphore_acquire_async` fails, the callback is not called.

`robust_semaphore_cancel` and `robust_semaphore_release` cannot take back a unit that the worker thread already acquired: if they race with the acquisition of a unit, `on_acquire_complete` can still be called with `ROBUST_SEMAPHORE_ACQUIRE_OK`. After `robust_semaphore_release` the unit is given back as soon as `on_acquire_complete` returns; after `robust_semaphore_cancel` the unit is held until `robust_semaphore_release` is called.

`max_hold_ms` is measured from the moment the unit is acquired, including the time spent in `on_acquire_complete`. The worker thread cannot release the mutex while it is running `on_acquire_complete`, so a callback that runs longer than `max_hold_ms` delays the release until it returns.

`robust_semaphore_destroy` must not be called from `on_acquire_complete`, because it waits for the worker thread. If it is, it cannot wait for the worker thread: it logs an error and the worker thread frees the object when the callback returns.

## Exposed API

```c
#define ROBUST_SEMAPHORE_MAX_COUNT 63

#define ROBUST_SEMAPHORE_NO_TIMEOUT UINT32_MAX

#define ROBUST_SEMAPHORE_UNIT_INDEX_NONE UINT32_MAX

#define ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES \
    ROBUST_SEMAPHORE_ACQUIRE_OK, \
    ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT, \
    ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, \
    ROBUST_SEMAPHORE_ACQUIRE_ERROR

MU_DEFINE_ENUM(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES);

typedef struct ROBUST_SEMAPHORE_TAG* ROBUST_SEMAPHORE_HANDLE;

typedef void (*ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE)(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index);

MOCKABLE_FUNCTION(, ROBUST_SEMAPHORE_HANDLE, robust_semaphore_create, const char*, name, uint32_t, max_count, const char*, security_descriptor);
MOCKABLE_FUNCTION(, void, robust_semaphore_destroy, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
MOCKABLE_FUNCTION(, int, robust_semaphore_acquire_async, ROBUST_SEMAPHORE_HANDLE, robust_semaphore, uint32_t, timeout_ms, uint32_t, max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE, on_acquire_complete, void*, on_acquire_complete_context);
MOCKABLE_FUNCTION(, void, robust_semaphore_cancel, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
MOCKABLE_FUNCTION(, void, robust_semaphore_release, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
```

### robust_semaphore_create

```c
MOCKABLE_FUNCTION(, ROBUST_SEMAPHORE_HANDLE, robust_semaphore_create, const char*, name, uint32_t, max_count, const char*, security_descriptor);
```

`robust_semaphore_create` creates a robust semaphore object for the semaphore `name` with `max_count` units. `name` follows the rules of kernel object names (for example `Global\my_semaphore` to share it between sessions). All the users of the same semaphore have to use the same `max_count`. `security_descriptor` is an optional security descriptor string (SDDL) applied to the mutexes when they are created; if it is `NULL` the mutexes get the default security descriptor.

**SRS_ROBUST_SEMAPHORE_01_001: [** If `name` is `NULL` then `robust_semaphore_create` shall fail and return `NULL`. **]**

**SRS_ROBUST_SEMAPHORE_01_002: [** If `max_count` is 0 then `robust_semaphore_create` shall fail and return `NULL`. **]**

**SRS_ROBUST_SEMAPHORE_01_003: [** If `max_count` is greater than `ROBUST_SEMAPHORE_MAX_COUNT` then `robust_semaphore_create` shall fail and return `NULL`. **]**

**SRS_ROBUST_SEMAPHORE_01_004: [** `robust_semaphore_create` shall allocate memory for the robust semaphore object. **]**

**SRS_ROBUST_SEMAPHORE_01_005: [** `robust_semaphore_create` shall initialize a lock by calling `srw_lock_ll_init`. **]**

**SRS_ROBUST_SEMAPHORE_01_006: [** If `security_descriptor` is not `NULL`, `robust_semaphore_create` shall convert it by calling `ConvertStringSecurityDescriptorToSecurityDescriptorA`. **]**

**SRS_ROBUST_SEMAPHORE_01_054: [** For each unit `i`, `robust_semaphore_create` shall: **]**

- **SRS_ROBUST_SEMAPHORE_01_007: [** build the mutex name `<name>_<i>` by calling `sprintf_char`. **]**

- **SRS_ROBUST_SEMAPHORE_01_008: [** create or open the mutex by calling `CreateMutexExA` with the converted security descriptor, the mutex name and `SYNCHRONIZE | MUTEX_MODIFY_STATE` access. **]**

- **SRS_ROBUST_SEMAPHORE_01_055: [** free the mutex name. **]**

**SRS_ROBUST_SEMAPHORE_01_009: [** `robust_semaphore_create` shall free the converted security descriptor by calling `LocalFree`. **]**

**SRS_ROBUST_SEMAPHORE_01_056: [** `robust_semaphore_create` shall create a manual reset cancel event by calling `CreateEventA`. **]**

**SRS_ROBUST_SEMAPHORE_01_057: [** `robust_semaphore_create` shall create a manual reset release event by calling `CreateEventA`. **]**

**SRS_ROBUST_SEMAPHORE_01_012: [** `robust_semaphore_create` shall succeed and return the robust semaphore object. **]**

**SRS_ROBUST_SEMAPHORE_01_013: [** If there are any failures then `robust_semaphore_create` shall close the handles it created by calling `CloseHandle`, free the memory it allocated and return `NULL`. **]**

### robust_semaphore_destroy

```c
MOCKABLE_FUNCTION(, void, robust_semaphore_destroy, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
```

`robust_semaphore_destroy` releases the unit, waits for the worker thread to end (if one was started) and frees the robust semaphore object. If an acquisition is still waiting, `on_acquire_complete` is called with `ROBUST_SEMAPHORE_ACQUIRE_CANCELLED` before `robust_semaphore_destroy` returns.

**SRS_ROBUST_SEMAPHORE_01_014: [** If `robust_semaphore` is `NULL` then `robust_semaphore_destroy` shall return. **]**

**SRS_ROBUST_SEMAPHORE_01_015: [** `robust_semaphore_destroy` shall acquire the lock in exclusive mode. **]**

**SRS_ROBUST_SEMAPHORE_01_061: [** `robust_semaphore_destroy` shall mark the object as being destroyed and release the current acquisition as `robust_semaphore_release` does. **]**

**SRS_ROBUST_SEMAPHORE_01_017: [** If `robust_semaphore_destroy` is called on the worker thread, it shall mark the worker thread as the one that frees the object, release the lock and return. **]**

**SRS_ROBUST_SEMAPHORE_01_062: [** Otherwise `robust_semaphore_destroy` shall release the lock and, if a worker thread was started, wait for it to end by calling `ThreadAPI_Join`. **]**

**SRS_ROBUST_SEMAPHORE_01_053: [** If `ThreadAPI_Join` fails, `robust_semaphore_destroy` shall return without freeing the object, because the worker thread may still use it. **]**

**SRS_ROBUST_SEMAPHORE_01_019: [** `robust_semaphore_destroy` shall close the mutexes and the events by calling `CloseHandle`. **]**

**SRS_ROBUST_SEMAPHORE_01_058: [** `robust_semaphore_destroy` shall deinitialize the lock by calling `srw_lock_ll_deinit`. **]**

**SRS_ROBUST_SEMAPHORE_01_059: [** `robust_semaphore_destroy` shall free the robust semaphore object. **]**

### robust_semaphore_acquire_async

```c
MOCKABLE_FUNCTION(, int, robust_semaphore_acquire_async, ROBUST_SEMAPHORE_HANDLE, robust_semaphore, uint32_t, timeout_ms, uint32_t, max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE, on_acquire_complete, void*, on_acquire_complete_context);
```

`robust_semaphore_acquire_async` starts waiting for a unit. `timeout_ms` is how long to wait for a unit (0 tries once, `ROBUST_SEMAPHORE_NO_TIMEOUT` waits until a unit is free). `max_hold_ms` is how long the unit may be held before the worker thread releases it early (`ROBUST_SEMAPHORE_NO_TIMEOUT` for no limit).

On success, `on_acquire_complete` is called exactly once on the worker thread, which can happen before `robust_semaphore_acquire_async` returns.

**SRS_ROBUST_SEMAPHORE_01_020: [** If `robust_semaphore` is `NULL` then `robust_semaphore_acquire_async` shall fail and return a non-zero value. **]**

**SRS_ROBUST_SEMAPHORE_01_021: [** If `on_acquire_complete` is `NULL` then `robust_semaphore_acquire_async` shall fail and return a non-zero value. **]**

**SRS_ROBUST_SEMAPHORE_01_022: [** `robust_semaphore_acquire_async` shall acquire the lock in exclusive mode. **]**

**SRS_ROBUST_SEMAPHORE_01_063: [** If the object is being destroyed, `robust_semaphore_acquire_async` shall fail and return a non-zero value. **]**

**SRS_ROBUST_SEMAPHORE_01_024: [** If there is a current acquisition that was not released or ended, `robust_semaphore_acquire_async` shall fail and return a non-zero value. **]**

**SRS_ROBUST_SEMAPHORE_01_064: [** `robust_semaphore_acquire_async` shall store `timeout_ms`, `max_hold_ms`, `on_acquire_complete` and `on_acquire_complete_context` as the new current acquisition. **]**

**SRS_ROBUST_SEMAPHORE_01_065: [** If the worker thread is running, `robust_semaphore_acquire_async` shall leave the new acquisition to it. **]**

**SRS_ROBUST_SEMAPHORE_01_066: [** Otherwise `robust_semaphore_acquire_async` shall: **]**

- **SRS_ROBUST_SEMAPHORE_01_067: [** if a worker thread was started before, wait for it to end by calling `ThreadAPI_Join`. **]**

- **SRS_ROBUST_SEMAPHORE_01_068: [** start a new worker thread by calling `ThreadAPI_Create` with `robust_semaphore_worker_thread` and mark it as running. **]**

**SRS_ROBUST_SEMAPHORE_01_026: [** `robust_semaphore_acquire_async` shall release the lock and succeed and return 0. **]**

**SRS_ROBUST_SEMAPHORE_01_069: [** If `ThreadAPI_Join` or `ThreadAPI_Create` fails, `robust_semaphore_acquire_async` shall forget the new acquisition and the worker thread, release the lock and return a non-zero value. **]**

### robust_semaphore_cancel

```c
MOCKABLE_FUNCTION(, void, robust_semaphore_cancel, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
```

`robust_semaphore_cancel` stops waiting for a unit. It has no effect if the unit was already acquired (use `robust_semaphore_release` to give it back).

**SRS_ROBUST_SEMAPHORE_01_028: [** If `robust_semaphore` is `NULL` then `robust_semaphore_cancel` shall return. **]**

**SRS_ROBUST_SEMAPHORE_01_029: [** `robust_semaphore_cancel` shall acquire the lock in exclusive mode. **]**

**SRS_ROBUST_SEMAPHORE_01_030: [** If the current acquisition was not released and did not get a unit, `robust_semaphore_cancel` shall mark it as cancelled and signal the cancel event by calling `SetEvent`. **]**

**SRS_ROBUST_SEMAPHORE_01_031: [** `robust_semaphore_cancel` shall release the lock. **]**

### robust_semaphore_release

```c
MOCKABLE_FUNCTION(, void, robust_semaphore_release, ROBUST_SEMAPHORE_HANDLE, robust_semaphore);
```

`robust_semaphore_release` gives back the unit of the current acquisition, or stops waiting for one. An acquisition that holds a unit ends right away (the worker thread releases the mutex after `robust_semaphore_release` returns). An acquisition that is still waiting ends when its `on_acquire_complete` is called with `ROBUST_SEMAPHORE_ACQUIRE_CANCELLED`. Calling it multiple times has no further effect.

**SRS_ROBUST_SEMAPHORE_01_032: [** If `robust_semaphore` is `NULL` then `robust_semaphore_release` shall return. **]**

**SRS_ROBUST_SEMAPHORE_01_033: [** `robust_semaphore_release` shall acquire the lock in exclusive mode. **]**

**SRS_ROBUST_SEMAPHORE_01_034: [** If there is a current acquisition that was not released, `robust_semaphore_release` shall mark it as released and signal the cancel event and the release event by calling `SetEvent`. **]**

**SRS_ROBUST_SEMAPHORE_01_051: [** If the current acquisition holds a unit, `robust_semaphore_release` shall end it. **]**

**SRS_ROBUST_SEMAPHORE_01_035: [** `robust_semaphore_release` shall release the lock. **]**

### robust_semaphore_worker_thread

```c
static int robust_semaphore_worker_thread(void* context);
```

`robust_semaphore_worker_thread` runs the acquisitions of the robust semaphore object one at a time and returns when there is no new acquisition.

**SRS_ROBUST_SEMAPHORE_01_036: [** `robust_semaphore_worker_thread` shall record the id of the worker thread by calling `GetCurrentThreadId`. **]**

**SRS_ROBUST_SEMAPHORE_01_037: [** If there is a new acquisition, `robust_semaphore_worker_thread` shall take it and reset the cancel event and the release event by calling `ResetEvent`. **]**

**SRS_ROBUST_SEMAPHORE_01_038: [** If the acquisition was already cancelled or released, `robust_semaphore_worker_thread` shall end it and call `on_acquire_complete` with `ROBUST_SEMAPHORE_ACQUIRE_CANCELLED` and `ROBUST_SEMAPHORE_UNIT_INDEX_NONE` without waiting for a unit. **]**

**SRS_ROBUST_SEMAPHORE_01_039: [** Otherwise `robust_semaphore_worker_thread` shall wait for the cancel event or any of the mutexes by calling `WaitForMultipleObjects` with the cancel event followed by the `max_count` mutexes, `bWaitAll` `FALSE` and `timeout_ms` as timeout (`INFINITE` if `timeout_ms` is `ROBUST_SEMAPHORE_NO_TIMEOUT`). **]**

**SRS_ROBUST_SEMAPHORE_01_040: [** If `WaitForMultipleObjects` returns `WAIT_OBJECT_0` then `robust_semaphore_worker_thread` shall end the acquisition and call `on_acquire_complete` with `ROBUST_SEMAPHORE_ACQUIRE_CANCELLED` and `ROBUST_SEMAPHORE_UNIT_INDEX_NONE`. **]**

**SRS_ROBUST_SEMAPHORE_01_041: [** If `WaitForMultipleObjects` returns `WAIT_OBJECT_0 + 1 + i` or `WAIT_ABANDONED_0 + 1 + i` then `robust_semaphore_worker_thread` shall record the time the unit was acquired by calling `timer_global_get_elapsed_ms`, mark the acquisition as holding unit `i` and call `on_acquire_complete` with `ROBUST_SEMAPHORE_ACQUIRE_OK` and `i`. **]**

**SRS_ROBUST_SEMAPHORE_01_042: [** If `WaitForMultipleObjects` returns `WAIT_TIMEOUT` then `robust_semaphore_worker_thread` shall end the acquisition and call `on_acquire_complete` with `ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT` and `ROBUST_SEMAPHORE_UNIT_INDEX_NONE`. **]**

**SRS_ROBUST_SEMAPHORE_01_043: [** If `WaitForMultipleObjects` returns any other value then `robust_semaphore_worker_thread` shall end the acquisition and call `on_acquire_complete` with `ROBUST_SEMAPHORE_ACQUIRE_ERROR` and `ROBUST_SEMAPHORE_UNIT_INDEX_NONE`. **]**

**SRS_ROBUST_SEMAPHORE_01_044: [** `robust_semaphore_worker_thread` shall end an acquisition that did not get a unit before calling its `on_acquire_complete`, so that `on_acquire_complete` can start a new acquisition. **]**

**SRS_ROBUST_SEMAPHORE_01_045: [** If a unit was acquired, after `on_acquire_complete` returns, `robust_semaphore_worker_thread` shall wait for the release event by calling `WaitForSingleObject` with what is left of `max_hold_ms` since the unit was acquired (measured by calling `timer_global_get_elapsed_ms`, 0 if `max_hold_ms` already passed, `INFINITE` if `max_hold_ms` is `ROBUST_SEMAPHORE_NO_TIMEOUT`). **]**

**SRS_ROBUST_SEMAPHORE_01_046: [** If waiting for the release event times out then `robust_semaphore_worker_thread` shall release the mutex early by calling `ReleaseMutex`. **]**

**SRS_ROBUST_SEMAPHORE_01_047: [** If a unit was acquired, `robust_semaphore_worker_thread` shall release the mutex by calling `ReleaseMutex` and, if the acquisition still holds the unit (it was not released), end it. **]**

**SRS_ROBUST_SEMAPHORE_01_070: [** If there is no new acquisition, `robust_semaphore_worker_thread` shall, while still holding the lock, mark the worker thread as not running and clear the recorded worker thread id. **]**

**SRS_ROBUST_SEMAPHORE_01_071: [** `robust_semaphore_worker_thread` shall then release the lock and return 0 without taking the lock again. **]**

**SRS_ROBUST_SEMAPHORE_01_049: [** If `robust_semaphore_destroy` was called on the worker thread, before returning `robust_semaphore_worker_thread` shall close its own thread handle by calling `CloseHandle`. **]**

**SRS_ROBUST_SEMAPHORE_01_060: [** If `robust_semaphore_destroy` was called on the worker thread, before returning `robust_semaphore_worker_thread` shall free the robust semaphore object as `robust_semaphore_destroy` does. **]**
