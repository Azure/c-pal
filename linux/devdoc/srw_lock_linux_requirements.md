# `srw_lock_linux` requirements

## Overview

`srw_lock_linux` is an implementation of slim read-write for linux.

## Design

`srw_lock_linux` used for synchronize resources across threads. It contains `exclusive mode` for write lock and `shared mode` for read lock. Acquire APIs will apply the required type of lock to `SRWLOCK` and get blocked if it cannot. Try acquire APIs will fail if the equivalent acquire APIs get blocked.

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

**SRS_SRW_LOCK_LINUX_07_001: [** `srw_lock_create` shall allocate memory for `SRW_LOCK_HANDLE_DATA`. **]**

**SRS_SRW_LOCK_LINUX_07_002: [** `srw_lock_create` shall copy the `lock_name`. **]**

**SRS_SRW_LOCK_LINUX_07_003: [** `srw_lock_create` shall initialized the `SRWLOCK` by calling `pthread_rwlock_init`. **]**

**SRS_SRW_LOCK_LINUX_07_006: [** If initializing lock failed, `srw_lock_create` shall fail and return `NULL`. **]**

**SRS_SRW_LOCK_LINUX_07_004: [** `srw_lock_create` shall succeed and return a non-`NULL` value. **]**

**SRS_SRW_LOCK_LINUX_07_005: [** If there are any failures, `srw_lock_create` shall fail and return `NULL`. **]**

### srw_lock_acquire_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_acquire_exclusive, SRW_LOCK_HANDLE, handle);
```

`srw_lock_acquire_exclusive` acquires the lock in exclusive (writer) mode.

**SRS_SRW_LOCK_LINUX_07_007: [** If `handle` is `NULL`, `srw_lock_acquire_exclusive` shall return. **]**

**SRS_SRW_LOCK_LINUX_07_008: [** `srw_lock_acquire_exclusive` shall lock the `SRWLOCK` for writing by calling `pthread_rwlock_wrlock`. **]**

### srw_lock_try_acquire_exclusive
```c
MOCKABLE_FUNCTION(, SRW_LOCK_TRY_ACQUIRE_RESULT, srw_lock_try_acquire_exclusive, SRW_LOCK_HANDLE, handle);
```

`srw_lock_try_acquire_exclusive` attempts to acquire the lock in exclusive (writer) mode.

**SRS_SRW_LOCK_LINUX_07_009: [** If `handle` is `NULL`, `srw_lock_try_acquire_exclusive` shall fail and return `SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_LINUX_07_010: [** Otherwise `srw_lock_acquire_exclusive` shall apply a write lock on `SRWLOCK` only if no other threads are currently holding the `SRWLOCK` by calling `pthread_rwlock_trywrlock`. **]**

**SRS_SRW_LOCK_LINUX_07_011: [** If `pthread_rwlock_trywrlock` returns 0, `srw_lock_acquire_exclusive` shall return `SRW_LOCK_TRY_ACQUIRE_OK`. **]**

**SRS_SRW_LOCK_LINUX_07_012: [** Otherwise, `srw_lock_acquire_exclusive` shall return `SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

### srw_lock_release_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_release_exclusive, SRW_LOCK_HANDLE, handle);
```

`srw_lock_release_exclusive` releases the underlying `SRWLOCK` from exclusive (write) mode.

**SRS_SRW_LOCK_LINUX_07_013: [** If `handle` is `NULL`, `srw_lock_release_exclusive` shall return. **]**

**SRS_SRW_LOCK_LINUX_07_014: [** `srw_lock_release_exclusive` shall release the write lock by calling `pthread_rwlock_unlock`. **]**


### srw_lock_acquire_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_acquire_shared, SRW_LOCK_HANDLE, handle);
```

`srw_lock_acquire_shared` acquires the SRWLOCK in shared (read) mode.

**SRS_SRW_LOCK_LINUX_07_015: [** If `handle` is `NULL`, `srw_lock_acquire_shared` shall return. **]**

**SRS_SRW_LOCK_LINUX_07_016: [** `srw_lock_acquire_shared` shall apply a read lock to `SRWLOCK` by calling `pthread_rwlock_rdlock`. **]**

### srw_lock_try_acquire_shared
```c
MOCKABLE_FUNCTION(, SRW_LOCK_TRY_ACQUIRE_RESULT, srw_lock_try_acquire_shared, SRW_LOCK_HANDLE, handle);
```

`srw_lock_try_acquire_shared` attempts to acquire the SRWLOCK in shared (read) mode.

**SRS_SRW_LOCK_LINUX_07_017: [** If `handle` is `NULL` then `srw_lock_try_acquire_shared` shall fail and return `SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_LINUX_07_018: [** Otherwise `srw_lock_try_acquire_shared` shall apply a read lock on `SRWLOCK` if there's no writers hold the lock and no writers blocked on the lock by calling `pthread_rwlock_tryrdlock`. **]**

**SRS_SRW_LOCK_LINUX_07_019: [** If `pthread_rwlock_tryrdlock` returns 0, `srw_lock_try_acquire_shared` shall return `SRW_LOCK_TRY_ACQUIRE_OK`. **]**

**SRS_SRW_LOCK_LINUX_07_020: [** Otherwise, `srw_lock_try_acquire_shared` shall return `SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

### srw_lock_release_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_release_shared, SRW_LOCK_HANDLE, handle);
```

`srw_lock_release_exclusive` releases the underlying `SRWLOCK` from shared (read) mode.

**SRS_SRW_LOCK_LINUX_07_021: [** If `handle` is `NULL`, `srw_lock_release_shared` shall return. **]**

**SRS_SRW_LOCK_LINUX_07_022: [** `srw_lock_release_shared` shall release the read lock by calling `pthread_rwlock_unlock`. **]**


### srw_lock_destroy
```c
MOCKABLE_FUNCTION(, void, srw_lock_destroy, SRW_LOCK_HANDLE, handle);
```

`srw_lock_destroy` frees all used resources.

**SRS_SRW_LOCK_LINUX_07_023: [** If `handle` is `NULL` then `srw_lock_destroy` shall return. **]**

**SRS_SRW_LOCK_LINUX_07_024: [** `srw_lock_destroy` shall free stored lock name. **]**

**SRS_SRW_LOCK_LINUX_07_025: [** `srw_lock_destroy` shall destroy the `SRWLOCK` by calling `pthread_rwlock_destroy`. **]**

**SRS_SRW_LOCK_LINUX_07_026: [** `srw_lock_destroy` shall free the lock handle. **]**
