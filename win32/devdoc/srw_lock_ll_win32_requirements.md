# `srw_lock_ll_win32` requirements

## Overview

`srw_lock_ll_win32` is the implementation of a slim reader writer lock (`srw_lock_ll` interface). On Windows it simply wraps `SRWLOCK`.

References:

[https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)

## Exposed API

For Windows the type `SRW_LOCK_LL` is defined to be `SRWLOCK`.

```c
typedef SRWLOCK SRW_LOCK_LL;
```

```c
#define SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES \
    SRW_LOCK_LL_TRY_ACQUIRE_OK, \
    SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, \
    SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS

MU_DEFINE_ENUM(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES)

MOCKABLE_FUNCTION(, void, srw_lock_ll_init, SRW_LOCK_LL*, srw_lock_ll);
MOCKABLE_FUNCTION(, void, srw_lock_ll_deinit, SRW_LOCK_LL*, srw_lock_ll);

/*writer APIs*/
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_exclusive, SRW_LOCK_LL*, srw_lock_ll);

/*reader APIs*/
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_shared, SRW_LOCK_LL*, srw_lock_ll);
```

### srw_lock_ll_init
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_init, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_init` initializes a slim reader writer lock.

**SRS_SRW_LOCK_LL_01_001: [** If `srw_lock_ll` is `NULL`, `srw_lock_ll_init` shall fail and return a non-zero value. **]**

**SRS_SRW_LOCK_LL_01_002: [** Otherwise, `srw_lock_ll_init` shall call `InitializeSRWLock`. **]**

**SRS_SRW_LOCK_LL_01_003: [** `srw_lock_ll_init` shall succeed and return 0. **]**

### srw_lock_ll_deinit
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_deinit, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_deinit` deinitializes the slim reader writer lock.

**SRS_SRW_LOCK_LL_01_004: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_deinit` shall return. **]**

**SRS_SRW_LOCK_LL_01_005: [** Otherwise, `srw_lock_ll_deinit` shall return. **]**

### srw_lock_ll_acquire_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_exclusive` acquires the slim reader writer lock in exclusive (writer) mode.

**SRS_SRW_LOCK_LL_01_006: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_acquire_exclusive` shall return. **]**

**SRS_SRW_LOCK_LL_01_007: [** `srw_lock_ll_acquire_exclusive` shall call `AcquireSRWLockExclusive`. **]**

### srw_lock_ll_try_acquire_exclusive
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_exclusive` attempts to acquire the slim reader writer lock in exclusive (writer) mode.

**SRS_SRW_LOCK_LL_01_008: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_try_acquire_exclusive` shall fail and return `SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_LL_01_009: [** Otherwise `srw_lock_ll_try_acquire_exclusive` shall call `TryAcquireSRWLockExclusive`. **]**

**SRS_SRW_LOCK_LL_01_011: [** If `TryAcquireSRWLockExclusive` returns `FALSE`, `srw_lock_ll_try_acquire_exclusive` shall return `SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

**SRS_SRW_LOCK_LL_01_010: [** If `TryAcquireSRWLockExclusive` returns `TRUE`, `srw_lock_ll_try_acquire_exclusive` shall return `SRW_LOCK_LL_TRY_ACQUIRE_OK`. **]**

### srw_lock_ll_release_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_release_exclusive` releases the underlying slim reader writer lock from exclusive (write) mode.

**SRS_SRW_LOCK_LL_01_012: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_release_exclusive` shall return. **]**

**SRS_SRW_LOCK_LL_01_013: [** `srw_lock_ll_release_exclusive` shall call `ReleaseSRWLockExclusive`. **]**

### srw_lock_ll_acquire_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_shared` acquires the slim reader writer lock in shared (read) mode.

**SRS_SRW_LOCK_LL_01_014: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_acquire_shared` shall return. **]**

**SRS_SRW_LOCK_LL_01_015: [** `srw_lock_ll_acquire_shared` shall call `AcquireSRWLockShared`. **]**

### srw_lock_ll_try_acquire_shared
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_shared` attempts to acquire the slim reader writer lock in shared (read) mode.

**SRS_SRW_LOCK_LL_01_016: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_try_acquire_shared` shall fail and return `SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_LL_01_017: [** Otherwise `srw_lock_ll_try_acquire_shared` shall call `TryAcquireSRWLockShared`. **]**

**SRS_SRW_LOCK_LL_01_018: [** If `TryAcquireSRWLockShared` returns `FALSE`, `srw_lock_ll_try_acquire_shared` shall return `SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

**SRS_SRW_LOCK_LL_01_019: [** If `TryAcquireSRWLockShared` returns `TRUE`, `srw_lock_ll_try_acquire_shared` shall return `SRW_LOCK_LL_TRY_ACQUIRE_OK`. **]**

### srw_lock_ll_release_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_release_shared` releases the underlying slim reader writer lock from shared (read) mode.

**SRS_SRW_LOCK_LL_01_020: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_release_shared` shall return. **]**

**SRS_SRW_LOCK_LL_01_021: [** `srw_lock_ll_release_shared` shall call `ReleaseSRWLockShared`. **]**
