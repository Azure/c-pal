// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>              // for NULL, free, malloc
#include <stdbool.h>

#include <pthread.h>

#include "c_logging/xlogging.h"

#include "c_pal/srw_lock.h"

typedef struct SRW_LOCK_HANDLE_DATA_TAG
{
    pthread_rwlock_t rwlock;
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
        pthread_rwlock_init(&result->rwlock, NULL);
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
        pthread_rwlock_wrlock(&handle->rwlock);
    }
}

SRW_LOCK_TRY_ACQUIRE_RESULT srw_lock_try_acquire_exclusive(SRW_LOCK_HANDLE handle)
{
    SRW_LOCK_TRY_ACQUIRE_RESULT result;

    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
        result = SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        if (pthread_rwlock_trywrlock(&handle->rwlock) == 0)
        {
            result = SRW_LOCK_TRY_ACQUIRE_OK;
        }
        else
        {
            result = SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
    }
    return result;
}

void srw_lock_release_exclusive(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        pthread_rwlock_unlock(&handle->rwlock);
    }
}

void srw_lock_acquire_shared(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        pthread_rwlock_rdlock(&handle->rwlock);
    }
}

SRW_LOCK_TRY_ACQUIRE_RESULT srw_lock_try_acquire_shared(SRW_LOCK_HANDLE handle)
{
    SRW_LOCK_TRY_ACQUIRE_RESULT result;

    if (handle == NULL)
    {
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
        result = SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        if (pthread_rwlock_tryrdlock(&handle->rwlock) == 0)
        {
            result = SRW_LOCK_TRY_ACQUIRE_OK;
        }
        else
        {
            result = SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
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
        pthread_rwlock_unlock(&handle->rwlock);
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
        pthread_rwlock_destroy(&handle->rwlock);
        free(handle);
    }
}

