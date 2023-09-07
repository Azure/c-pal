// Copyright (C) Microsoft Corporation. All rights reserved.

#ifndef SRW_LOCK_LL_H
#define SRW_LOCK_LL_H

#ifdef __cplusplus
#include <cstdbool>
#else
#include <stdbool.h>
#endif

#ifdef WIN32
#include "windows.h"
#endif // WINDOWS

#include "macro_utils/macro_utils.h"
#include "umock_c/umock_c_prod.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifdef WIN32
    typedef SRWLOCK SRW_LOCK_LL;
#else
    typedef pthread_rwlock_t SRW_LOCK_LL;
#endif

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

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // SRW_LOCK_LL_H
