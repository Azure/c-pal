// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SRW_LOCK_H
#define SRW_LOCK_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif

#endif
