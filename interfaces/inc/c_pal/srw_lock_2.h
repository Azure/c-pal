// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SRW_LOCK_2_H
#define SRW_LOCK_2_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#ifdef _MSC_VER
#include "windows.h"
#endif

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
typedef SRWLOCK SRW_LOCK_2;
#else
#endif

#define SRW_LOCK_2_TRY_ACQUIRE_RESULT_VALUES \
    SRW_LOCK_2_TRY_ACQUIRE_OK, \
    SRW_LOCK_2_TRY_ACQUIRE_COULD_NOT_ACQUIRE, \
    SRW_LOCK_2_TRY_ACQUIRE_INVALID_ARGS

MU_DEFINE_ENUM(SRW_LOCK_2_TRY_ACQUIRE_RESULT, SRW_LOCK_2_TRY_ACQUIRE_RESULT_VALUES)

MOCKABLE_FUNCTION(, void, srw_lock_2_init, SRW_LOCK_2*, srw_lock_2);

/*writer APIs*/
MOCKABLE_FUNCTION(, void, srw_lock_2_acquire_exclusive, SRW_LOCK_2*, srw_lock_2);
MOCKABLE_FUNCTION(, SRW_LOCK_2_TRY_ACQUIRE_RESULT, srw_lock_2_try_acquire_exclusive, SRW_LOCK_2*, srw_lock_2);
MOCKABLE_FUNCTION(, void, srw_lock_2_release_exclusive, SRW_LOCK_2*, srw_lock_2);

/*reader APIs*/
MOCKABLE_FUNCTION(, void, srw_lock_2_acquire_shared, SRW_LOCK_2*, srw_lock_2);
MOCKABLE_FUNCTION(, SRW_LOCK_2_TRY_ACQUIRE_RESULT, srw_lock_2_try_acquire_shared, SRW_LOCK_2*, srw_lock_2);
MOCKABLE_FUNCTION(, void, srw_lock_2_release_shared, SRW_LOCK_2*, srw_lock_2);

MOCKABLE_FUNCTION(, void, srw_lock_2_deinit, SRW_LOCK_2*, handle);

#ifdef __cplusplus
}
#endif

#endif // SRW_LOCK_2_H
