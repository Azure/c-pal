# `srw_lock_ll` requirements

## Overview

`srw_lock_ll` is the implementation of a slim reader writer lock.

References:

[https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)

## Exposed API

```c
#define SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES \
    SRW_LOCK_LL_TRY_ACQUIRE_OK, \
    SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, \
    SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS

MU_DEFINE_ENUM(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES)

MOCKABLE_FUNCTION_WITH_RETURNS(, int, srw_lock_ll_init, SRW_LOCK_LL*, srw_lock_ll)(0, MU_FAILURE);
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

Note that the type `SRW_LOCK_LL` is defined individually for each platform.

### srw_lock_ll_init
```c
MOCKABLE_FUNCTION_WITH_RETURNS(, int, srw_lock_ll_init, SRW_LOCK_LL*, srw_lock_ll)(0, MU_FAILURE);
```

`srw_lock_ll_init` initializes a slim reader writer lock.

### srw_lock_ll_deinit
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_deinit, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_deinit` deinitializes the slim reader writer lock.

### srw_lock_ll_acquire_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_exclusive` acquires the slim reader writer lock in exclusive (writer) mode.


### srw_lock_ll_try_acquire_exclusive
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_exclusive` attempts to acquire the slim reader writer lock in exclusive (writer) mode.

### srw_lock_ll_release_exclusive
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_exclusive, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_release_exclusive` releases the underlying slim reader writer lock from exclusive (write) mode.

### srw_lock_ll_acquire_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_acquire_shared` acquires the slim reader writer lock in shared (read) mode.

### srw_lock_ll_try_acquire_shared
```c
MOCKABLE_FUNCTION(, SRW_LOCK_LL_TRY_ACQUIRE_RESULT, srw_lock_ll_try_acquire_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_try_acquire_shared` attempts to acquire the slim reader writer lock in shared (read) mode.

### srw_lock_ll_release_shared
```c
MOCKABLE_FUNCTION(, void, srw_lock_ll_release_shared, SRW_LOCK_LL*, srw_lock_ll);
```

`srw_lock_ll_release_shared` releases the underlying slim reader writer lock from shared (read) mode.
