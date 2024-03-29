# `srw_lock` requirements

## Overview

`srw_lock` is a wrapper over a `SRWLOCK` with the additional benefit of having some statistics printed.

## Exposed API

```c
typedef struct SRW_LOCK_HANDLE_DATA_TAG* SRW_LOCK_HANDLE;

#define SRW_LOCK_TRY_ACQUIRE_RESULT_VALUES \
    SRW_LOCK_TRY_ACQUIRE_OK, \
    SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, \
    SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS

MU_DEFINE_ENUM(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_RESULT_VALUES)

MOCKABLE_FUNCTION(, SRW_LOCK_HANDLE, srw_lock_create, bool, do_statistics, const char*, lock_name);

/*writer APIs*/
MOCKABLE_FUNCTION(, void, srw_lock_acquire_exclusive, SRW_LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, SRW_LOCK_TRY_ACQUIRE_RESULT, srw_lock_try_acquire_exclusive, SRW_LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, void, srw_lock_release_exclusive, SRW_LOCK_HANDLE, handle);

/*reader APIs*/
MOCKABLE_FUNCTION(, void, srw_lock_acquire_shared, SRW_LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, SRW_LOCK_TRY_ACQUIRE_RESULT, srw_lock_try_acquire_shared, SRW_LOCK_HANDLE, handle);
MOCKABLE_FUNCTION(, void, srw_lock_release_shared, SRW_LOCK_HANDLE, handle);

MOCKABLE_FUNCTION(, void, srw_lock_destroy, SRW_LOCK_HANDLE, handle);
```

### srw_lock_create
```c
MOCKABLE_FUNCTION(, SRW_LOCK_HANDLE, srw_lock_create, bool, do_statistics, const char*, lock_name);
```

`srw_lock_create` creates a new `SRW_LOCK_HANDLE`.

**SRS_SRW_LOCK_02_001: [** `srw_lock_create` shall allocate memory for `SRW_LOCK_HANDLE`. **]**

**SRS_SRW_LOCK_02_015: [** `srw_lock_create` shall call `InitializeSRWLock`. **]**

**SRS_SRW_LOCK_02_023: [** If `do_statistics` is `true` then `srw_lock_create` shall copy `lock_name`.  **]**

**SRS_SRW_LOCK_02_024: [** If `do_statistics` is `true` then `srw_lock_create` shall create a new `TIMER_HANDLE` by calling `timer_create_new`. **]**

**SRS_SRW_LOCK_02_003: [** `srw_lock_create` shall succeed and return a non-`NULL` value. **]**

**SRS_SRW_LOCK_02_004: [** If there are any failures then `srw_lock_create` shall fail and return `NULL`. **]**

### srw_lock_acquire_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_acquire_exclusive, SRW_LOCK_HANDLE, handle);
```

`srw_lock_acquire_exclusive` acquires the lock in exclusive (writer) mode.

**SRS_SRW_LOCK_02_022: [** If `handle` is `NULL` then `srw_lock_acquire_exclusive` shall return. **]**

**SRS_SRW_LOCK_02_006: [** `srw_lock_acquire_exclusive` shall call `AcquireSRWLockExclusive`. **]**

**SRS_SRW_LOCK_02_025: [** If `do_statistics` is `true` and if the timer created has recorded more than `TIME_BETWEEN_STATISTICS_LOG` seconds then statistics will be logged and the timer shall be started again. **]**

### srw_lock_try_acquire_exclusive
```c
MOCKABLE_FUNCTION(, SRW_LOCK_TRY_ACQUIRE_RESULT, srw_lock_try_acquire_exclusive, SRW_LOCK_HANDLE, handle);
```

`srw_lock_try_acquire_exclusive` attempts to acquire the lock in exclusive (writer) mode.

**SRS_SRW_LOCK_01_006: [** If `handle` is `NULL` then `srw_lock_try_acquire_exclusive` shall fail and return `SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_01_007: [** Otherwise `srw_lock_acquire_exclusive` shall call `TryAcquireSRWLockExclusive`. **]**

**SRS_SRW_LOCK_01_008: [** If `TryAcquireSRWLockExclusive` returns `FALSE`, `srw_lock_acquire_exclusive` shall return `SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

**SRS_SRW_LOCK_01_009: [** If `TryAcquireSRWLockExclusive` returns `TRUE`, `srw_lock_acquire_exclusive` shall return `SRW_LOCK_TRY_ACQUIRE_OK`. **]**

**SRS_SRW_LOCK_01_010: [** If `do_statistics` is `true` and if the timer created has recorded more than `TIME_BETWEEN_STATISTICS_LOG` seconds then statistics will be logged and the timer shall be started again. **]**

### srw_lock_release_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_release_exclusive, SRW_LOCK_HANDLE, handle);
```

`srw_lock_release_exclusive` releases the underlying `SRWLOCK` from exclusive (write) mode.

**SRS_SRW_LOCK_02_009: [** If `handle` is `NULL` then `srw_lock_release_exclusive` shall return. **]**

**SRS_SRW_LOCK_02_010: [** `srw_lock_release_exclusive` shall call `ReleaseSRWLockExclusive`. **]**


### srw_lock_acquire_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_acquire_shared, SRW_LOCK_HANDLE, handle);
```

`srw_lock_acquire_shared` acquires the SRWLOCK in shared (read) mode.

**SRS_SRW_LOCK_02_017: [** If `handle` is `NULL` then `srw_lock_acquire_shared` shall return. **]**

**SRS_SRW_LOCK_02_018: [** `srw_lock_acquire_shared` shall call `AcquireSRWLockShared`. **]**

**SRS_SRW_LOCK_02_026: [** If `do_statistics` is `true` and the timer created has recorded more than `TIME_BETWEEN_STATISTICS_LOG` seconds then statistics will be logged and the timer shall be started again. **]**

### srw_lock_try_acquire_shared
```c
MOCKABLE_FUNCTION(, SRW_LOCK_TRY_ACQUIRE_RESULT, srw_lock_try_acquire_shared, SRW_LOCK_HANDLE, handle);
```

`srw_lock_try_acquire_shared` attempts to acquire the SRWLOCK in shared (read) mode.

**SRS_SRW_LOCK_01_001: [** If `handle` is `NULL` then `srw_lock_try_acquire_shared` shall fail and return `SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_01_002: [** Otherwise `srw_lock_try_acquire_shared` shall call `TryAcquireSRWLockShared`. **]**

**SRS_SRW_LOCK_01_003: [** If `TryAcquireSRWLockShared` returns `FALSE`, `srw_lock_try_acquire_shared` shall return `SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

**SRS_SRW_LOCK_01_004: [** If `TryAcquireSRWLockShared` returns `TRUE`, `srw_lock_try_acquire_shared` shall return `SRW_LOCK_TRY_ACQUIRE_OK`. **]**

**SRS_SRW_LOCK_01_005: [** If `do_statistics` is `true` and the timer created has recorded more than `TIME_BETWEEN_STATISTICS_LOG` seconds then statistics will be logged and the timer shall be started again. **]**


### srw_lock_release_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_release_shared, SRW_LOCK_HANDLE, handle);
```

**SRS_SRW_LOCK_02_020: [** If `handle` is `NULL` then `srw_lock_release_shared` shall return. **]**

**SRS_SRW_LOCK_02_021: [** `srw_lock_release_shared` shall call `ReleaseSRWLockShared`. **]**


### srw_lock_destroy
```c
MOCKABLE_FUNCTION(, void, srw_lock_destroy, SRW_LOCK_HANDLE, handle);
```

`srw_lock_destroy` frees all used resources.

**SRS_SRW_LOCK_02_011: [** If `handle` is `NULL` then `srw_lock_destroy` shall return. **]**

**SRS_SRW_LOCK_02_012: [** `srw_lock_destroy` shall free all used resources. **]**

