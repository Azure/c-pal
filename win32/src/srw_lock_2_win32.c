// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <stdbool.h>

#include "windows.h"

#include "c_logging/xlogging.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/timer.h"
#include "c_pal/string_utils.h"

#include "c_pal/srw_lock_2.h"

void srw_lock_2_init(SRW_LOCK_2* srw_lock_2)
{
    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
    }
    else
    {
        InitializeSRWLock(srw_lock_2);
    }
}

void srw_lock_2_acquire_exclusive(SRW_LOCK_2* srw_lock_2)
{
    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
    }
    else
    {
        AcquireSRWLockExclusive(srw_lock_2);
    }
}

SRW_LOCK_2_TRY_ACQUIRE_RESULT srw_lock_2_try_acquire_exclusive(SRW_LOCK_2* srw_lock_2)
{
    SRW_LOCK_2_TRY_ACQUIRE_RESULT result;

    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
        result = SRW_LOCK_2_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        if (!TryAcquireSRWLockExclusive(srw_lock_2))
        {
            result = SRW_LOCK_2_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
        else
        {
            result = SRW_LOCK_2_TRY_ACQUIRE_OK;
        }
    }

    return result;
}

void srw_lock_2_release_exclusive(SRW_LOCK_2* srw_lock_2)
{
    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
    }
    else
    {
        ReleaseSRWLockExclusive(srw_lock_2);
    }
}

void srw_lock_2_acquire_shared(SRW_LOCK_2* srw_lock_2)
{
    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
    }
    else
    {
        AcquireSRWLockShared(srw_lock_2);
    }
}

SRW_LOCK_2_TRY_ACQUIRE_RESULT srw_lock_2_try_acquire_shared(SRW_LOCK_2* srw_lock_2)
{
    SRW_LOCK_2_TRY_ACQUIRE_RESULT result;

    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
        result = SRW_LOCK_2_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        if (!TryAcquireSRWLockShared(srw_lock_2))
        {
            result = SRW_LOCK_2_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
        else
        {
            result = SRW_LOCK_2_TRY_ACQUIRE_OK;
        }
    }

    return result;
}

void srw_lock_2_release_shared(SRW_LOCK_2* srw_lock_2)
{
    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
    }
    else
    {
        ReleaseSRWLockShared(srw_lock_2);
    }
}

void srw_lock_2_deinit(SRW_LOCK_2* srw_lock_2)
{
    if (srw_lock_2 == NULL)
    {
        LogError("invalid argument SRW_LOCK_2* srw_lock_2=%p", srw_lock_2);
    }
    else
    {
        // do nothing
    }
}

