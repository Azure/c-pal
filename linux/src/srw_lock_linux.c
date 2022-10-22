// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>              // for NULL, free, malloc
#include <inttypes.h>
#include <stdbool.h>

#include "c_logging/xlogging.h"

#include "c_pal/srw_lock.h"

#define TIME_BETWEEN_STATISTICS_LOG 600 /*in seconds, so every 10 minutes*/

typedef struct SRW_LOCK_HANDLE_DATA_TAG
{
    //SRWLOCK lock;
    int not_used_yet;
} SRW_LOCK_HANDLE_DATA;

SRW_LOCK_HANDLE srw_lock_create(bool do_statistics, const char* lock_name)
{
    (void)do_statistics;
    (void)lock_name;
    SRW_LOCK_HANDLE result;

    result = malloc(sizeof(SRW_LOCK_HANDLE_DATA));
    if (result == NULL)
    {
        /*return as is*/
        LogError("failure in malloc(sizeof(SRW_LOCK_HANDLE_DATA)=%zu)", sizeof(SRW_LOCK_HANDLE_DATA));
    }
    else
    {
        //InitializeSRWLock(&result->lock);
    }
allOk:;
    return result;

}

void srw_lock_acquire_exclusive(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
    }
}

SRW_LOCK_TRY_ACQUIRE_RESULT srw_lock_try_acquire_exclusive(SRW_LOCK_HANDLE handle)
{
    SRW_LOCK_TRY_ACQUIRE_RESULT result;

    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_01_006: [ If handle is NULL then srw_lock_try_acquire_exclusive shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
        result = SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        result = SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
    }

    return result;
}

void srw_lock_release_exclusive(SRW_LOCK_HANDLE handle)
{
    /*Codes_SRS_SRW_LOCK_02_009: [ If handle is NULL then srw_lock_release_exclusive shall return. ]*/
    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
    }
}

void srw_lock_acquire_shared(SRW_LOCK_HANDLE handle)
{
    /*Codes_SRS_SRW_LOCK_02_017: [ If handle is NULL then srw_lock_acquire_shared shall return. ]*/
    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
    }
}

SRW_LOCK_TRY_ACQUIRE_RESULT srw_lock_try_acquire_shared(SRW_LOCK_HANDLE handle)
{
    SRW_LOCK_TRY_ACQUIRE_RESULT result;

    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_01_001: [ If handle is NULL then srw_lock_try_acquire_shared shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
        result = SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        result = SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
    }
    return result;
}

void srw_lock_release_shared(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
    }
}

void srw_lock_destroy(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("invalid arguments SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        /*Codes_SRS_SRW_LOCK_02_012: [ srw_lock_destroy shall free all used resources. ]*/
        /*there's no deinit for a SRW, since it is a static construct*/
        free(handle);
    }
}

