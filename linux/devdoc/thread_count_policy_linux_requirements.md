# `thread_count_policy_linux` requirements

## Overview

`thread_count_policy_linux` decides when `execution_engine_linux` adds a worker thread and when it retires one. It holds no threads and takes no locks: the caller passes in the current counts and times, and serializes the calls.

- The maximum thread count is `max_thread_count`, or 500 (but at least `min_thread_count`) when `max_thread_count` is 0.
- A worker thread is added only when work waits in the ready queue, no worker thread is idle, the thread count is below the maximum, and no work was taken from the ready queue for 50 ms. A worker thread that blocks in a callback stops making progress, so this keeps work from starving behind blocking callbacks without adding threads for short bursts of work.
- After failing to add a worker thread, the next attempt waits 100 ms.
- While the thread count is above `min_thread_count`, a worker thread that has been idle for 60 s is retired.

Times are in milliseconds on the clock the caller uses for `now_ms`.

## Exposed API

```c
#define THREAD_COUNT_POLICY_NEVER UINT64_MAX

typedef struct THREAD_COUNT_POLICY_TAG* THREAD_COUNT_POLICY_HANDLE;

MOCKABLE_FUNCTION(, THREAD_COUNT_POLICY_HANDLE, thread_count_policy_create, uint32_t, min_thread_count, uint32_t, max_thread_count);
MOCKABLE_FUNCTION(, void, thread_count_policy_destroy, THREAD_COUNT_POLICY_HANDLE, thread_count_policy);
MOCKABLE_FUNCTION(, bool, thread_count_policy_can_add_thread, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, thread_count);
MOCKABLE_FUNCTION(, void, thread_count_policy_on_progress, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint64_t, now_ms);
MOCKABLE_FUNCTION(, void, thread_count_policy_on_add_thread_failed, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint64_t, now_ms);
MOCKABLE_FUNCTION(, uint64_t, thread_count_policy_get_add_thread_time, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, queued_work_count, uint32_t, starting_thread_count, uint32_t, idle_thread_count, uint32_t, thread_count);
MOCKABLE_FUNCTION(, uint64_t, thread_count_policy_get_retire_thread_time, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, thread_count, uint32_t, idle_thread_count, uint64_t, oldest_idle_since_ms, uint64_t, now_ms);
```

### thread_count_policy_create

```c
MOCKABLE_FUNCTION(, THREAD_COUNT_POLICY_HANDLE, thread_count_policy_create, uint32_t, min_thread_count, uint32_t, max_thread_count);
```

`thread_count_policy_create` creates a policy for a pool of `min_thread_count` to `max_thread_count` threads. A `max_thread_count` of 0 means the default maximum.

**SRS_THREAD_COUNT_POLICY_LINUX_42_001: [** If `max_thread_count` is not 0 and is less than `min_thread_count`, `thread_count_policy_create` shall fail and return `NULL`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_002: [** `thread_count_policy_create` shall allocate a new policy. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_003: [** `thread_count_policy_create` shall store `min_thread_count` as the minimum thread count. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_004: [** If `max_thread_count` is 0, `thread_count_policy_create` shall use 500 as the maximum thread count, or `min_thread_count` if it is larger. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_005: [** If `max_thread_count` is not 0, `thread_count_policy_create` shall use `max_thread_count` as the maximum thread count. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_006: [** `thread_count_policy_create` shall set the last progress time to 0. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_007: [** `thread_count_policy_create` shall set the earliest time to add a thread to 0. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_008: [** `thread_count_policy_create` shall succeed and return the policy. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_009: [** If any error occurs, `thread_count_policy_create` shall fail and return `NULL`. **]**

### thread_count_policy_destroy

```c
MOCKABLE_FUNCTION(, void, thread_count_policy_destroy, THREAD_COUNT_POLICY_HANDLE, thread_count_policy);
```

**SRS_THREAD_COUNT_POLICY_LINUX_42_010: [** If `thread_count_policy` is `NULL`, `thread_count_policy_destroy` shall return. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_011: [** `thread_count_policy_destroy` shall free `thread_count_policy`. **]**

### thread_count_policy_can_add_thread

```c
MOCKABLE_FUNCTION(, bool, thread_count_policy_can_add_thread, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, thread_count);
```

`thread_count_policy_can_add_thread` tells whether a pool of `thread_count` threads can get another thread.

**SRS_THREAD_COUNT_POLICY_LINUX_42_012: [** If `thread_count_policy` is `NULL`, `thread_count_policy_can_add_thread` shall return `false`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_013: [** `thread_count_policy_can_add_thread` shall return whether `thread_count` is less than the maximum thread count. **]**

### thread_count_policy_on_progress

```c
MOCKABLE_FUNCTION(, void, thread_count_policy_on_progress, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint64_t, now_ms);
```

The caller calls `thread_count_policy_on_progress` when the ready queue becomes non-empty, when a worker thread takes work from the ready queue and when a worker thread is added.

**SRS_THREAD_COUNT_POLICY_LINUX_42_014: [** If `thread_count_policy` is `NULL`, `thread_count_policy_on_progress` shall return. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_015: [** `thread_count_policy_on_progress` shall set the last progress time to `now_ms`. **]**

### thread_count_policy_on_add_thread_failed

```c
MOCKABLE_FUNCTION(, void, thread_count_policy_on_add_thread_failed, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint64_t, now_ms);
```

**SRS_THREAD_COUNT_POLICY_LINUX_42_016: [** If `thread_count_policy` is `NULL`, `thread_count_policy_on_add_thread_failed` shall return. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_017: [** `thread_count_policy_on_add_thread_failed` shall set the earliest time to add a thread to `now_ms` plus 100. **]**

### thread_count_policy_get_add_thread_time

```c
MOCKABLE_FUNCTION(, uint64_t, thread_count_policy_get_add_thread_time, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, queued_work_count, uint32_t, starting_thread_count, uint32_t, idle_thread_count, uint32_t, thread_count);
```

`thread_count_policy_get_add_thread_time` returns the time at which the caller adds a worker thread if nothing changes until then, or `THREAD_COUNT_POLICY_NEVER` if no worker thread is needed. `queued_work_count` is the number of work objects in the ready queue, `starting_thread_count` the number of worker threads created that did not start taking work yet, `idle_thread_count` the number of idle worker threads and `thread_count` the number of worker threads.

**SRS_THREAD_COUNT_POLICY_LINUX_42_018: [** If `thread_count_policy` is `NULL`, `thread_count_policy_get_add_thread_time` shall return `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_019: [** If `queued_work_count` is not greater than `starting_thread_count`, `thread_count_policy_get_add_thread_time` shall return `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_020: [** If `idle_thread_count` is not 0, `thread_count_policy_get_add_thread_time` shall return `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_021: [** If `thread_count` is not less than the maximum thread count, `thread_count_policy_get_add_thread_time` shall return `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_022: [** Otherwise, `thread_count_policy_get_add_thread_time` shall return the later of the last progress time plus 50 and the earliest time to add a thread. **]**

### thread_count_policy_get_retire_thread_time

```c
MOCKABLE_FUNCTION(, uint64_t, thread_count_policy_get_retire_thread_time, THREAD_COUNT_POLICY_HANDLE, thread_count_policy, uint32_t, thread_count, uint32_t, idle_thread_count, uint64_t, oldest_idle_since_ms, uint64_t, now_ms);
```

`thread_count_policy_get_retire_thread_time` returns the time at which the caller retires the worker thread that has been idle the longest if nothing changes until then, or `THREAD_COUNT_POLICY_NEVER` if no worker thread can be retired. `oldest_idle_since_ms` is the time at which the worker thread idle the longest became idle; it is ignored when `idle_thread_count` is 0.

**SRS_THREAD_COUNT_POLICY_LINUX_42_023: [** If `thread_count_policy` is `NULL`, `thread_count_policy_get_retire_thread_time` shall return `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_024: [** If `thread_count` is not greater than the minimum thread count, `thread_count_policy_get_retire_thread_time` shall return `THREAD_COUNT_POLICY_NEVER`. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_025: [** If `idle_thread_count` is 0, `thread_count_policy_get_retire_thread_time` shall return `now_ms` plus 60000. **]**

**SRS_THREAD_COUNT_POLICY_LINUX_42_026: [** Otherwise, `thread_count_policy_get_retire_thread_time` shall return `oldest_idle_since_ms` plus 60000. **]**
