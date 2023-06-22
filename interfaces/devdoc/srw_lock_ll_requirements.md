# `srw_lock_ll` requirements

## Overview

`srw_lock_ll` is the implementation of a slim reader writer lock (on Windows it simply wraps`SRWLOCK`).

## Exposed API

```c
#define SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES \
    SRW_LOCK_LL_TRY_ACQUIRE_OK, \
    SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, \
    SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS

typedef void* SRW_LOCK_LL;

MU_DEFINE_ENUM(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES)

MOCKABLE_FUNCTION(, void, srw_lock_ll_init);
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

If `srw_lock_ll` is `NULL`, `srw_lock_ll_init` shall return.

Otherwise, `srw_lock_ll_init` shall call `InitializeSRWLock`.

### srw_lock_ll_deinit
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_deinit, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_deinit` frees deinitializes the slim reader writer lock.

If `srw_lock_ll` is `NULL` then `srw_lock_ll_deinit` shall return.

Otherwise, `srw_lock_ll_deinit` shall return.

### srw_lock_ll_acquire_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_exclusive` acquires the slim reader writer lock in exclusive (writer) mode.

If `srw_lock_ll` is `NULL` then `srw_lock_ll_acquire_exclusive` shall return.

`srw_lock_ll_acquire_exclusive` shall call `AcquireSRWLockExclusive`.

### srw_lock_ll_try_acquire_exclusive
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_exclusive` attempts to acquire the slim reader writer lock in exclusive (writer) mode.

If `srw_lock_ll` is `NULL` then `srw_lock_ll_try_acquire_exclusive` shall fail and return `SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS`.

Otherwise `srw_lock_ll_try_acquire_exclusive` shall call `TryAcquireSRWLockExclusive`.

If `TryAcquireSRWLockExclusive` returns `FALSE`, `srw_lock_ll_try_acquire_exclusive` shall return `SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE`.

If `TryAcquireSRWLockExclusive` returns `TRUE`, `srw_lock_ll_try_acquire_exclusive` shall return `SRW_LOCK_LL_TRY_ACQUIRE_OK`.

### srw_lock_ll_release_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_release_exclusive` releases the underlying slim reader writer lock from exclusive (write) mode.

If `srw_lock_ll` is `NULL` then `srw_lock_ll_release_exclusive` shall return.

`srw_lock_ll_release_exclusive` shall call `ReleaseSRWLockExclusive`.

### srw_lock_ll_acquire_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_shared` acquires the SRWLOCK in shared (read) mode.

If `srw_lock_ll` is `NULL` then `srw_lock_ll_acquire_shared` shall return.

`srw_lock_ll_acquire_shared` shall call `AcquireSRWLockShared`.

### srw_lock_ll_try_acquire_shared
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_shared` attempts to acquire the slim reader writer lock in shared (read) mode.

If `srw_lock_ll` is `NULL` then `srw_lock_ll_try_acquire_shared` shall fail and return `SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS`.

Otherwise `srw_lock_ll_try_acquire_shared` shall call `TryAcquireSRWLockShared`.

If `TryAcquireSRWLockShared` returns `FALSE`, `srw_lock_ll_try_acquire_shared` shall return `SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE`.

If `TryAcquireSRWLockShared` returns `TRUE`, `srw_lock_ll_try_acquire_shared` shall return `SRW_LOCK_LL_TRY_ACQUIRE_OK`.

### srw_lock_ll_release_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_shared, SRW_LOCK_LL*, srw_lock_ll);
```

If `srw_lock_ll` is `NULL` then `srw_lock_ll_release_shared` shall return.

`srw_lock_ll_release_shared` shall call `ReleaseSRWLockShared`.
