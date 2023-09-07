# `srw_lock_ll_linux` requirements

## Overview

`srw_lock_ll_linux` is the implementation of a slim reader writer lock (`srw_lock_ll` interface). On Linux it simply wraps `pthread_rwlock_t`.


## Exposed API

For Linux the type `SRW_LOCK_LL` is defined to be `pthread_rwlock_t`.

```c
typedef pthread_rwlock_t SRW_LOCK_LL;
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

**SRS_SRW_LOCK_LL_11_001: [** If `srw_lock_ll` is `NULL`, `srw_lock_ll_init` shall fail and return a non-zero value. **]**

**SRS_SRW_LOCK_LL_11_002: [** Otherwise, `srw_lock_ll_init` shall call `pthread_rwlock_init`. **]**

**SRS_SRW_LOCK_LL_11_022: [** If `pthread_rwlock_init` returns a non-zero value, `srw_lock_ll_init` shall fail and return a non-zero value. **]**

**SRS_SRW_LOCK_LL_11_003: [** otherwise, `srw_lock_ll_init` shall succeed and return 0. **]**

### srw_lock_ll_deinit
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_deinit, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_deinit` deinitializes the slim reader writer lock.

**SRS_SRW_LOCK_LL_11_004: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_deinit` shall return. **]**

**SRS_SRW_LOCK_LL_11_005: [** Otherwise, `srw_lock_ll_deinit` shall call `pthread_rwlock_destroy` and return. **]**

### srw_lock_ll_acquire_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_exclusive` acquires the slim reader writer lock in exclusive (writer) mode.

**SRS_SRW_LOCK_LL_11_006: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_acquire_exclusive` shall return. **]**

**SRS_SRW_LOCK_LL_11_007: [** `srw_lock_ll_acquire_exclusive` shall call `pthread_rwlock_wrlock`. **]**

### srw_lock_ll_try_acquire_exclusive
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_exclusive` attempts to acquire the slim reader writer lock in exclusive (writer) mode.

**SRS_SRW_LOCK_LL_11_008: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_try_acquire_exclusive` shall fail and return `SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_LL_11_009: [** Otherwise `srw_lock_ll_try_acquire_exclusive` shall call `pthread_rwlock_trywrlock`. **]**

**SRS_SRW_LOCK_LL_11_011: [** If `pthread_rwlock_trywrlock` returns a non-zero value, `srw_lock_ll_try_acquire_exclusive` shall return `SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

**SRS_SRW_LOCK_LL_11_010: [** If `pthread_rwlock_trywrlock` returns zero, `srw_lock_ll_try_acquire_exclusive` shall return `SRW_LOCK_LL_TRY_ACQUIRE_OK`. **]**

### srw_lock_ll_release_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_release_exclusive` releases the underlying slim reader writer lock from exclusive (write) mode.

**SRS_SRW_LOCK_LL_11_012: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_release_exclusive` shall return. **]**

**SRS_SRW_LOCK_LL_11_013: [** `srw_lock_ll_release_exclusive` shall call `pthread_rwlock_unlock`. **]**

### srw_lock_ll_acquire_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_shared` acquires the slim reader writer lock in shared (read) mode.

**SRS_SRW_LOCK_LL_11_014: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_acquire_shared` shall return. **]**

**SRS_SRW_LOCK_LL_11_015: [** `srw_lock_ll_acquire_shared` shall call `pthread_rwlock_rdlock`. **]**

### srw_lock_ll_try_acquire_shared
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_shared` attempts to acquire the slim reader writer lock in shared (read) mode.

**SRS_SRW_LOCK_LL_11_016: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_try_acquire_shared` shall fail and return `SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS`. **]**

**SRS_SRW_LOCK_LL_11_017: [** Otherwise `srw_lock_ll_try_acquire_shared` shall call `pthread_rwlock_tryrdlock`. **]**

**SRS_SRW_LOCK_LL_11_018: [** If `pthread_rwlock_tryrdlock` returns a non-zero value, `srw_lock_ll_try_acquire_shared` shall return `SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE`. **]**

**SRS_SRW_LOCK_LL_11_019: [** If `pthread_rwlock_tryrdlock` returns zero, `srw_lock_ll_try_acquire_shared` shall return `SRW_LOCK_LL_TRY_ACQUIRE_OK`. **]**

### srw_lock_ll_release_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_release_shared` releases the underlying slim reader writer lock from shared (read) mode.

**SRS_SRW_LOCK_LL_11_020: [** If `srw_lock_ll` is `NULL` then `srw_lock_ll_release_shared` shall return. **]**

**SRS_SRW_LOCK_LL_11_021: [** `srw_lock_ll_release_shared` shall call `pthread_rwlock_unlock`. **]**
