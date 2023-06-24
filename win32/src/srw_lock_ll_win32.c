// Copyright (C) Microsoft Corporation. All rights reserved.

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/srw_lock_ll.h"

int srw_lock_ll_init(SRW_LOCK_LL* srw_lock_ll)
{
    int result;

    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_001: [ If srw_lock_ll is NULL, srw_lock_ll_init shall fail and return a non-zero value. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
        result = MU_FAILURE;
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_002: [ Otherwise, srw_lock_ll_init shall call InitializeSRWLock. ] */
        InitializeSRWLock(srw_lock_ll);

        /* Codes_SRS_SRW_LOCK_LL_01_003: [ srw_lock_ll_init shall succeed and return 0. ] */
        result = 0;
    }

    return result;
}

void srw_lock_ll_deinit(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_004: [ If srw_lock_ll is NULL then srw_lock_ll_deinit shall return. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_005: [ Otherwise, srw_lock_ll_deinit shall return. ] */
    }
}


void srw_lock_ll_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_006: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_exclusive shall return. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_007: [ srw_lock_ll_acquire_exclusive shall call AcquireSRWLockExclusive. ] */
        AcquireSRWLockExclusive(srw_lock_ll);
    }
}

SRW_LOCK_LL_TRY_ACQUIRE_RESULT srw_lock_ll_try_acquire_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result;

    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_008: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_exclusive shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
        result = SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_009: [ Otherwise srw_lock_ll_try_acquire_exclusive shall call TryAcquireSRWLockExclusive. ] */
        if (!TryAcquireSRWLockExclusive(srw_lock_ll))
        {
            /* Codes_SRS_SRW_LOCK_LL_01_011: [ If TryAcquireSRWLockExclusive returns FALSE, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ] */
            result = SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
        else
        {
            /* Codes_SRS_SRW_LOCK_LL_01_010: [ If TryAcquireSRWLockExclusive returns TRUE, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ] */
            result = SRW_LOCK_LL_TRY_ACQUIRE_OK;
        }
    }
        
    return result;
}

void srw_lock_ll_release_exclusive(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_012: [ If srw_lock_ll is NULL then srw_lock_ll_release_exclusive shall return. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_013: [ srw_lock_ll_release_exclusive shall call ReleaseSRWLockExclusive. ] */
        ReleaseSRWLockExclusive(srw_lock_ll);
    }
}

void srw_lock_ll_acquire_shared(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_014: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_shared shall return. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_015: [ srw_lock_ll_acquire_shared shall call AcquireSRWLockShared. ] */
        AcquireSRWLockShared(srw_lock_ll);
    }
}

SRW_LOCK_LL_TRY_ACQUIRE_RESULT srw_lock_ll_try_acquire_shared(SRW_LOCK_LL* srw_lock_ll)
{
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result;

    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_016: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_shared shall fail and return SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
        result = SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_017: [ Otherwise srw_lock_ll_try_acquire_shared shall call TryAcquireSRWLockShared. ] */
        if (!TryAcquireSRWLockShared(srw_lock_ll))
        {
            /* Codes_SRS_SRW_LOCK_LL_01_018: [ If TryAcquireSRWLockShared returns FALSE, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ] */
            result = SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
        else
        {
            /* Codes_SRS_SRW_LOCK_LL_01_019: [ If TryAcquireSRWLockShared returns TRUE, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ] */
            result = SRW_LOCK_LL_TRY_ACQUIRE_OK;
        }
    }

    return result;
}

void srw_lock_ll_release_shared(SRW_LOCK_LL* srw_lock_ll)
{
    if (srw_lock_ll == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LL_01_020: [ If srw_lock_ll is NULL then srw_lock_ll_release_shared shall return. ] */
        LogError("Invalid arguments: SRW_LOCK_LL* srw_lock_ll=%p", srw_lock_ll);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LL_01_021: [ srw_lock_ll_release_shared shall call ReleaseSRWLockShared. ] */
        ReleaseSRWLockShared(srw_lock_ll);
    }
}
