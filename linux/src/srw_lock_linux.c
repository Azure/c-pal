// Copyright (C) Microsoft Corporation. All rights reserved.

#include <stdlib.h>              // for NULL, free, malloc
#include <stdbool.h>

#include <pthread.h>

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h" // IWYU pragma: keep
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/string_utils.h"
#include "c_pal/srw_lock.h"

typedef struct SRW_LOCK_HANDLE_DATA_TAG
{
    char* lock_name;
    pthread_rwlock_t rwlock;
} SRW_LOCK_HANDLE_DATA;

SRW_LOCK_HANDLE srw_lock_create(bool do_statistics, const char* lock_name)
{
    (void)do_statistics;
    SRW_LOCK_HANDLE result;

    /* Codes_SRS_SRW_LOCK_LINUX_07_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE_DATA. ]*/
    result = malloc(sizeof(SRW_LOCK_HANDLE_DATA));
    if (result == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_005: [ If there are any failures, srw_lock_create shall fail and return NULL. ]*/
        LogError("failure in malloc(sizeof(SRW_LOCK_HANDLE_DATA)=%zu, lock_name: %s", sizeof(SRW_LOCK_HANDLE_DATA), MU_P_OR_NULL(lock_name) );
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_002: [ srw_lock_create shall copy the lock_name. ]*/
        result->lock_name = sprintf_char("%s", MU_P_OR_NULL(lock_name));
        if (result->lock_name == NULL)
        {
            /* Codes_SRS_SRW_LOCK_LINUX_07_005: [ If there are any failures then srw_lock_create shall fail and return NULL. ]*/
            LogError("Failure allocating lock_name %s", MU_P_OR_NULL(lock_name));
        }
        else
        {
            int ret;
            /* Codes_SRS_SRW_LOCK_LINUX_07_003: [ srw_lock_create shall initialized the SRWLOCK by calling pthread_rwlock_init. ]*/
            if ((ret = pthread_rwlock_init(&result->rwlock, NULL)) != 0)
            {
                /* Codes_SRS_SRW_LOCK_LINUX_07_006: [ If initializing lock failed, srw_lock_create shall fail and return NULL. ]*/
                LogError("Failure pthread_rwlock_init with %d lock_name %s", ret, MU_P_OR_NULL(lock_name));
            }
            else
            {
                goto allOk;
            }
            free(result->lock_name);
        }
        free(result);
        result = NULL;
    }
allOk:;
    /* Codes_SRS_SRW_LOCK_LINUX_07_004: [ srw_lock_create shall succeed and return a non-NULL value. ]*/
    return result;

}

void srw_lock_acquire_exclusive(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_007: [ If handle is NULL, srw_lock_acquire_exclusive shall return. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        int ret;
        /* Codes_SRS_SRW_LOCK_LINUX_07_008: [ srw_lock_acquire_exclusive shall lock the SRWLOCK for writing by calling pthread_rwlock_wrlock. ]*/
        if ((ret = pthread_rwlock_wrlock(&handle->rwlock)) != 0)
        {
            LogError("failure in pthread_rwlock_wrlock %d", ret);
        }
    }
}

SRW_LOCK_TRY_ACQUIRE_RESULT srw_lock_try_acquire_exclusive(SRW_LOCK_HANDLE handle)
{
    SRW_LOCK_TRY_ACQUIRE_RESULT result;

    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_009: [ If handle is NULL, srw_lock_try_acquire_exclusive shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
        result = SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_010: [ Otherwise srw_lock_acquire_exclusive shall apply a write lock on SRWLOCK only if no other threads are currently holding the SRWLOCK by calling pthread_rwlock_trywrlock. ]*/
        if (pthread_rwlock_trywrlock(&handle->rwlock) == 0)
        {
            /* Codes_SRS_SRW_LOCK_LINUX_07_011: [ If pthread_rwlock_trywrlock returns 0, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
            result = SRW_LOCK_TRY_ACQUIRE_OK;
        }
        else
        {
            /* Codes_SRS_SRW_LOCK_LINUX_07_012: [ Otherwise, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]*/
            result = SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
    }
    return result;
}

void srw_lock_release_exclusive(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_013: [ If handle is NULL, srw_lock_release_exclusive shall return. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_014: [ srw_lock_release_exclusive shall release the write lock by calling pthread_rwlock_unlock. ]*/
        pthread_rwlock_unlock(&handle->rwlock);
    }
}

void srw_lock_acquire_shared(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_015: [ If handle is NULL, srw_lock_acquire_shared shall return. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        int ret;
        /* Codes_SRS_SRW_LOCK_LINUX_07_016: [ srw_lock_acquire_shared shall apply a read lock to SRWLOCK by calling pthread_rwlock_rdlock. ]*/
        if ((ret = pthread_rwlock_rdlock(&handle->rwlock)) != 0)
        {
            LogError("failure in pthread_rwlock_rdlock %d", ret);
        }
    }
}

SRW_LOCK_TRY_ACQUIRE_RESULT srw_lock_try_acquire_shared(SRW_LOCK_HANDLE handle)
{
    SRW_LOCK_TRY_ACQUIRE_RESULT result;

    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_017: [ If handle is NULL, srw_lock_try_acquire_shared shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
        result = SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS;
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_018: [ Otherwise srw_lock_try_acquire_shared shall apply a read lock on SRWLOCK if there's no writers hold the lock and no writers blocked on the lock by calling pthread_rwlock_tryrdlock. ]*/
        if (pthread_rwlock_tryrdlock(&handle->rwlock) == 0)
        {
            /* Codes_SRS_SRW_LOCK_LINUX_07_019: [ If pthread_rwlock_tryrdlock returns 0, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
            result = SRW_LOCK_TRY_ACQUIRE_OK;
        }
        else
        {
            /* Codes_SRS_SRW_LOCK_LINUX_07_020: [ Otherwise, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]*/
            result = SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE;
        }
    }
    return result;
}

void srw_lock_release_shared(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_021: [ If handle is NULL, srw_lock_release_shared shall return. ]*/
        LogError("invalid argument SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        int ret;
        /* Codes_SRS_SRW_LOCK_LINUX_07_022: [ srw_lock_release_shared shall release the read lock by calling pthread_rwlock_unlock. ]*/
        if ((ret = pthread_rwlock_unlock(&handle->rwlock)) != 0)
        {
            LogError("failure in pthread_rwlock_unlock %d", ret);
        }
    }
}

void srw_lock_destroy(SRW_LOCK_HANDLE handle)
{
    if (handle == NULL)
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_023: [ If handle is NULL then srw_lock_destroy shall return. ]*/
        LogError("invalid arguments SRW_LOCK_HANDLE handle=%p", handle);
    }
    else
    {
        /* Codes_SRS_SRW_LOCK_LINUX_07_024: [ srw_lock_destroy shall free stored lock name. ]*/
        free(handle->lock_name);
        /* Codes_SRS_SRW_LOCK_LINUX_07_025: [ srw_lock_destroy shall destroy the SRWLOCK by calling pthread_rwlock_destroy. ]*/
        pthread_rwlock_destroy(&handle->rwlock);
        /* Codes_SRS_SRW_LOCK_LINUX_07_026: [ srw_lock_destroy shall free the lock handle. ]*/
        free(handle);
    }
}

