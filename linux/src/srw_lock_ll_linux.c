// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stddef.h>
#include <pthread.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/srw_lock_ll.h"

MU_DEFINE_ENUM_STRINGS(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES)

int srw_lock_ll_init(SRW_LOCK_LL* srw_lock_ll)
{
    int result;

    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_001: [ If srw_lock_ll is NULL, srw_lock_ll_init shall fail and return a non-zero value. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
        result = MU_FAILURE;
    }
    else
    {
        int ret;
        // Codes_SRS_SRW_LOCK_LL_11_002: [ Otherwise, srw_lock_ll_init shall call pthread_rwlock_init. ]
        if ((ret = pthread_rwlock_init(srw_lock_ll, NULL)) != 0)
        {
            // Codes_SRS_SRW_LOCK_LL_11_022: [ If pthread_rwlock_init returns a non-zero value, srw_lock_ll_init shall fail and return a non-zero value. ]
            LogError("Failure pthread_rwlock_init with %d", ret);
            result = MU_FAILURE;
        }
        else
        {
            // Codes_SRS_SRW_LOCK_LL_11_003: [ Otherwise, srw_lock_ll_init shall succeed and return 0. ]
            result = 0;
        }
    }
    return result;
}

void srw_lock_ll_deinit(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_004: [ If srw_lock_ll is NULL then srw_lock_ll_deinit shall return. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_005: [ Otherwise, srw_lock_ll_deinit shall call pthread_rwlock_destroy and return. ]
        pthread_rwlock_destroy(srw_lock_ll);
    }
}


void srw_lock_ll_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_006: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_exclusive shall return. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_007: [ srw_lock_ll_acquire_exclusive shall call pthread_rwlock_wrlock. ]
        (void)pthread_rwlock_wrlock(srw_lock_ll);
    }
}

SRW_LOCK_LL_TRY_ACQUIRE_RESULT srw_lock_ll_try_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result;

    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_008: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_exclusive shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
        result = SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_009: [ Otherwise srw_lock_ll_try_acquire_exclusive shall call pthread_rwlock_trywrlock. ]
        if (pthread_rwlock_trywrlock(srw_lock_ll) != 0)
        {
            // Codes_SRS_SRW_LOCK_LL_11_011: [ If pthread_rwlock_trywrlock returns a non-zero value, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]
            result = SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
        else
        {
            // Codes_SRS_SRW_LOCK_LL_11_010: [ If pthread_rwlock_trywrlock returns zero, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ]
            result = SRW_LOCK_LL_TRY_ACQUIRE_OK;
        }
    }
        
    return result;
}

void srw_lock_ll_release_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_012: [ If srw_lock_ll is NULL then srw_lock_ll_release_exclusive shall return. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_013: [ srw_lock_ll_release_exclusive shall call pthread_rwlock_unlock. ]
        pthread_rwlock_unlock(srw_lock_ll);
    }
}

void srw_lock_ll_acquire_shared(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_014: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_shared shall return. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_015: [ srw_lock_ll_acquire_shared shall call pthread_rwlock_rdlock. ]
        (void)pthread_rwlock_rdlock(srw_lock_ll);
    }
}

SRW_LOCK_LL_TRY_ACQUIRE_RESULT srw_lock_ll_try_acquire_shared(SRW_LOCK_LL* srw_lock_ll)
{
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result;

    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_016: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_shared shall fail and return SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
        result = SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_017: [ Otherwise srw_lock_ll_try_acquire_shared shall call pthread_rwlock_tryrdlock. ]
        if (pthread_rwlock_tryrdlock(srw_lock_ll) != 0)
        {
            // Codes_SRS_SRW_LOCK_LL_11_018: [ If pthread_rwlock_tryrdlock returns a non-zero value, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]
            result = SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
        else
        {
            // Codes_SRS_SRW_LOCK_LL_11_019: [ If pthread_rwlock_tryrdlock returns zero, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ]
            result = SRW_LOCK_LL_TRY_ACQUIRE_OK;
        }
    }

    return result;
}

void srw_lock_ll_release_shared(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        // Codes_SRS_SRW_LOCK_LL_11_020: [ If srw_lock_ll is NULL then srw_lock_ll_release_shared shall return. ]
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        // Codes_SRS_SRW_LOCK_LL_11_021: [ srw_lock_ll_release_shared shall call pthread_rwlock_unlock. ]
        (void)pthread_rwlock_unlock(srw_lock_ll);
    }
}
