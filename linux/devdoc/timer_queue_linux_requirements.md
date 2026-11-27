# `timer_queue_linux` requirements

## Overview

`timer_queue_linux` keeps the armed timers of `execution_engine_linux` in due time order, in a `min_heap`. A `TIMER_QUEUE_ENTRY` is placed inside the caller's timer structure (`CONTAINING_RECORD` gets the structure back).

Room for a timer is reserved when the timer is created (`timer_queue_reserve` can fail), so arming, disarming and popping timers cannot fail.

A periodic timer that is popped is armed again for its next period. If the timer fell behind by more than a period (for example because the caller did not pop it in time), its next due time is a period after the current time, so the timer fires once for the periods it missed instead of once per missed period.

`timer_queue_linux` is not thread-safe: the caller serializes the calls. Times are in milliseconds on the clock the caller uses for `now_ms` and `due_time_ms`.

## Exposed API

```c
#define TIMER_QUEUE_NO_DUE_TIME UINT64_MAX

typedef struct TIMER_QUEUE_ENTRY_TAG
{
    MIN_HEAP_ENTRY heap_entry;
    uint32_t period_ms;
} TIMER_QUEUE_ENTRY;

typedef struct TIMER_QUEUE_TAG* TIMER_QUEUE_HANDLE;

MOCKABLE_FUNCTION(, TIMER_QUEUE_HANDLE, timer_queue_create);
MOCKABLE_FUNCTION(, void, timer_queue_destroy, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, int, timer_queue_reserve, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, void, timer_queue_unreserve, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, void, timer_queue_entry_init, TIMER_QUEUE_ENTRY*, entry);
MOCKABLE_FUNCTION(, bool, timer_queue_arm, TIMER_QUEUE_HANDLE, timer_queue, TIMER_QUEUE_ENTRY*, entry, uint64_t, due_time_ms, uint32_t, period_ms);
MOCKABLE_FUNCTION(, void, timer_queue_disarm, TIMER_QUEUE_HANDLE, timer_queue, TIMER_QUEUE_ENTRY*, entry);
MOCKABLE_FUNCTION(, uint64_t, timer_queue_get_next_due_time, TIMER_QUEUE_HANDLE, timer_queue);
MOCKABLE_FUNCTION(, TIMER_QUEUE_ENTRY*, timer_queue_pop_due, TIMER_QUEUE_HANDLE, timer_queue, uint64_t, now_ms);
```

### timer_queue_create

```c
MOCKABLE_FUNCTION(, TIMER_QUEUE_HANDLE, timer_queue_create);
```

`timer_queue_create` creates an empty timer queue.

**SRS_TIMER_QUEUE_LINUX_42_001: [** `timer_queue_create` shall allocate a new timer queue by calling `malloc`. **]**

**SRS_TIMER_QUEUE_LINUX_42_002: [** `timer_queue_create` shall create the heap of armed timers by calling `min_heap_create`. **]**

**SRS_TIMER_QUEUE_LINUX_42_003: [** `timer_queue_create` shall succeed and return the timer queue. **]**

**SRS_TIMER_QUEUE_LINUX_42_004: [** If any error occurs, `timer_queue_create` shall free the resources it allocated, fail and return `NULL`. **]**

### timer_queue_destroy

```c
MOCKABLE_FUNCTION(, void, timer_queue_destroy, TIMER_QUEUE_HANDLE, timer_queue);
```

`timer_queue_destroy` frees the timer queue without accessing the timers still armed.

**SRS_TIMER_QUEUE_LINUX_42_005: [** If `timer_queue` is `NULL`, `timer_queue_destroy` shall return. **]**

**SRS_TIMER_QUEUE_LINUX_42_006: [** `timer_queue_destroy` shall destroy the heap of armed timers by calling `min_heap_destroy`. **]**

**SRS_TIMER_QUEUE_LINUX_42_007: [** `timer_queue_destroy` shall free `timer_queue`. **]**

### timer_queue_reserve

```c
MOCKABLE_FUNCTION(, int, timer_queue_reserve, TIMER_QUEUE_HANDLE, timer_queue);
```

`timer_queue_reserve` reserves room for one more timer.

**SRS_TIMER_QUEUE_LINUX_42_008: [** If `timer_queue` is `NULL`, `timer_queue_reserve` shall fail and return a non-zero value. **]**

**SRS_TIMER_QUEUE_LINUX_42_009: [** `timer_queue_reserve` shall reserve room in the heap of armed timers by calling `min_heap_reserve`. **]**

**SRS_TIMER_QUEUE_LINUX_42_010: [** If `min_heap_reserve` fails, `timer_queue_reserve` shall fail and return a non-zero value. **]**

**SRS_TIMER_QUEUE_LINUX_42_011: [** `timer_queue_reserve` shall succeed and return 0. **]**

### timer_queue_unreserve

```c
MOCKABLE_FUNCTION(, void, timer_queue_unreserve, TIMER_QUEUE_HANDLE, timer_queue);
```

`timer_queue_unreserve` gives back the room reserved for one timer.

**SRS_TIMER_QUEUE_LINUX_42_012: [** If `timer_queue` is `NULL`, `timer_queue_unreserve` shall return. **]**

**SRS_TIMER_QUEUE_LINUX_42_013: [** `timer_queue_unreserve` shall give back the room by calling `min_heap_unreserve`. **]**

### timer_queue_entry_init

```c
MOCKABLE_FUNCTION(, void, timer_queue_entry_init, TIMER_QUEUE_ENTRY*, entry);
```

`timer_queue_entry_init` initializes an entry as not armed.

**SRS_TIMER_QUEUE_LINUX_42_014: [** If `entry` is `NULL`, `timer_queue_entry_init` shall return. **]**

**SRS_TIMER_QUEUE_LINUX_42_015: [** `timer_queue_entry_init` shall initialize the heap entry of `entry` by calling `min_heap_entry_init`. **]**

**SRS_TIMER_QUEUE_LINUX_42_016: [** `timer_queue_entry_init` shall set the period of `entry` to 0. **]**

### timer_queue_arm

```c
MOCKABLE_FUNCTION(, bool, timer_queue_arm, TIMER_QUEUE_HANDLE, timer_queue, TIMER_QUEUE_ENTRY*, entry, uint64_t, due_time_ms, uint32_t, period_ms);
```

`timer_queue_arm` arms `entry` to be due at `due_time_ms` and then every `period_ms` (only once if `period_ms` is 0), replacing any previous arming, and returns whether `entry` is now the first timer to be due.

**SRS_TIMER_QUEUE_LINUX_42_017: [** If `timer_queue` is `NULL`, `timer_queue_arm` shall return `false`. **]**

**SRS_TIMER_QUEUE_LINUX_42_018: [** If `entry` is `NULL`, `timer_queue_arm` shall return `false`. **]**

**SRS_TIMER_QUEUE_LINUX_42_019: [** `timer_queue_arm` shall remove `entry` from the heap of armed timers by calling `min_heap_remove`. **]**

**SRS_TIMER_QUEUE_LINUX_42_020: [** `timer_queue_arm` shall set the period of `entry` to `period_ms`. **]**

**SRS_TIMER_QUEUE_LINUX_42_021: [** `timer_queue_arm` shall insert `entry` in the heap of armed timers with `due_time_ms` as key by calling `min_heap_insert`. **]**

**SRS_TIMER_QUEUE_LINUX_42_022: [** `timer_queue_arm` shall return whether `entry` is the first timer of the heap of armed timers obtained by calling `min_heap_peek`. **]**

### timer_queue_disarm

```c
MOCKABLE_FUNCTION(, void, timer_queue_disarm, TIMER_QUEUE_HANDLE, timer_queue, TIMER_QUEUE_ENTRY*, entry);
```

`timer_queue_disarm` disarms `entry` if it is armed.

**SRS_TIMER_QUEUE_LINUX_42_023: [** If `timer_queue` is `NULL`, `timer_queue_disarm` shall return. **]**

**SRS_TIMER_QUEUE_LINUX_42_024: [** If `entry` is `NULL`, `timer_queue_disarm` shall return. **]**

**SRS_TIMER_QUEUE_LINUX_42_025: [** `timer_queue_disarm` shall remove `entry` from the heap of armed timers by calling `min_heap_remove`. **]**

### timer_queue_get_next_due_time

```c
MOCKABLE_FUNCTION(, uint64_t, timer_queue_get_next_due_time, TIMER_QUEUE_HANDLE, timer_queue);
```

`timer_queue_get_next_due_time` returns when the first timer is due, or `TIMER_QUEUE_NO_DUE_TIME` if no timer is armed.

**SRS_TIMER_QUEUE_LINUX_42_026: [** If `timer_queue` is `NULL`, `timer_queue_get_next_due_time` shall return `TIMER_QUEUE_NO_DUE_TIME`. **]**

**SRS_TIMER_QUEUE_LINUX_42_027: [** `timer_queue_get_next_due_time` shall get the first timer by calling `min_heap_peek`. **]**

**SRS_TIMER_QUEUE_LINUX_42_028: [** If no timer is armed, `timer_queue_get_next_due_time` shall return `TIMER_QUEUE_NO_DUE_TIME`. **]**

**SRS_TIMER_QUEUE_LINUX_42_029: [** `timer_queue_get_next_due_time` shall return the due time of the first timer. **]**

### timer_queue_pop_due

```c
MOCKABLE_FUNCTION(, TIMER_QUEUE_ENTRY*, timer_queue_pop_due, TIMER_QUEUE_HANDLE, timer_queue, uint64_t, now_ms);
```

`timer_queue_pop_due` returns the first timer if it is due at `now_ms`, arming it again if it is periodic, or `NULL` if no timer is due.

**SRS_TIMER_QUEUE_LINUX_42_030: [** If `timer_queue` is `NULL`, `timer_queue_pop_due` shall return `NULL`. **]**

**SRS_TIMER_QUEUE_LINUX_42_031: [** `timer_queue_pop_due` shall get the first timer by calling `min_heap_peek`. **]**

**SRS_TIMER_QUEUE_LINUX_42_032: [** If no timer is armed, `timer_queue_pop_due` shall return `NULL`. **]**

**SRS_TIMER_QUEUE_LINUX_42_033: [** If the due time of the first timer is later than `now_ms`, `timer_queue_pop_due` shall return `NULL`. **]**

**SRS_TIMER_QUEUE_LINUX_42_034: [** `timer_queue_pop_due` shall remove the first timer from the heap of armed timers by calling `min_heap_remove`. **]**

**SRS_TIMER_QUEUE_LINUX_42_035: [** If the period of the timer is not 0: **]**

- **SRS_TIMER_QUEUE_LINUX_42_036: [** `timer_queue_pop_due` shall compute the next due time as the due time plus the period. **]**

- **SRS_TIMER_QUEUE_LINUX_42_037: [** If the next due time is not later than `now_ms`, `timer_queue_pop_due` shall use `now_ms` plus the period as the next due time. **]**

- **SRS_TIMER_QUEUE_LINUX_42_038: [** `timer_queue_pop_due` shall insert the timer in the heap of armed timers with the next due time as key by calling `min_heap_insert`. **]**

**SRS_TIMER_QUEUE_LINUX_42_039: [** `timer_queue_pop_due` shall return the timer. **]**
