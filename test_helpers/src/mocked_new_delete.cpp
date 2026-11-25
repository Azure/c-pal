// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstddef>
#include <new>

#include "c_pal/gballoc_hl_redirect.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/srw_lock.h"

#include "c_pal_test_helpers/mocked_new_delete.h"

// mocked_new_delete is enabled while the lock exists.
static SRW_LOCK_HANDLE g_lock = nullptr;

int mocked_new_delete_enable(void)
{
    int result;

    if (g_lock != nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_001: [ If mocked_new_delete is already enabled, mocked_new_delete_enable shall return 0. ]*/
        result = 0;
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_002: [ Otherwise: ]*/
        /*Codes_SRS_MOCKED_NEW_DELETE_16_003: [ mocked_new_delete_enable shall create the lock by calling srw_lock_create with false and an empty lock name. ]*/
        SRW_LOCK_HANDLE lock = srw_lock_create(false, "");
        if (lock == nullptr)
        {
            /*Codes_SRS_MOCKED_NEW_DELETE_16_006: [ If there are any failures, mocked_new_delete_enable shall fail and return a non-zero value. ]*/
            LogError("srw_lock_create failed");
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_MOCKED_NEW_DELETE_16_004: [ mocked_new_delete_enable shall mark mocked_new_delete as enabled. ]*/
            g_lock = lock;
            /*Codes_SRS_MOCKED_NEW_DELETE_16_005: [ mocked_new_delete_enable shall return 0 on success. ]*/
            result = 0;
        }
    }

    return result;
}

void mocked_new_delete_disable(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_007: [ If mocked_new_delete is disabled, mocked_new_delete_disable shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_008: [ Otherwise: ]*/
        SRW_LOCK_HANDLE lock = g_lock;
        /*Codes_SRS_MOCKED_NEW_DELETE_16_009: [ mocked_new_delete_disable shall mark mocked_new_delete as disabled. ]*/
        g_lock = nullptr;
        /*Codes_SRS_MOCKED_NEW_DELETE_16_010: [ mocked_new_delete_disable shall destroy the lock by calling srw_lock_destroy. ]*/
        srw_lock_destroy(lock);
    }
}

void mocked_new_delete_lock(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_011: [ If mocked_new_delete is disabled, mocked_new_delete_lock shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_012: [ Otherwise, mocked_new_delete_lock shall acquire the lock by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(g_lock);
    }
}

void mocked_new_delete_unlock(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_013: [ If mocked_new_delete is disabled, mocked_new_delete_unlock shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_014: [ Otherwise, mocked_new_delete_unlock shall release the lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(g_lock);
    }
}

void* operator new(size_t size)
{
    void* result;

    /*Codes_SRS_MOCKED_NEW_DELETE_16_015: [ If size is 0, operator new shall allocate 1 byte instead. ]*/
    size_t allocation_size = (size == 0) ? 1 : size;

    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_016: [ If mocked_new_delete is disabled, operator new shall allocate memory by calling gballoc_hl_malloc with the size to allocate. ]*/
        result = gballoc_hl_malloc(allocation_size);
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_017: [ Otherwise: ]*/
        /*Codes_SRS_MOCKED_NEW_DELETE_16_018: [ operator new shall acquire the lock by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(g_lock);
        /*Codes_SRS_MOCKED_NEW_DELETE_16_019: [ operator new shall allocate memory by calling mocked_new_delete_malloc with the size to allocate. ]*/
        result = mocked_new_delete_malloc(allocation_size);
        /*Codes_SRS_MOCKED_NEW_DELETE_16_020: [ operator new shall release the lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(g_lock);
    }

    if (result == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_022: [ If there are any failures, operator new shall throw std::bad_alloc. ]*/
        throw std::bad_alloc();
    }
    else
    {
        /* do nothing */
    }

    /*Codes_SRS_MOCKED_NEW_DELETE_16_021: [ operator new shall return the allocated memory on success. ]*/
    return result;
}

void* operator new[](size_t size)
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_023: [ operator new[] shall return the result of calling operator new with size. ]*/
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept
{
    if (ptr == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_024: [ If ptr is NULL, operator delete shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_025: [ Otherwise, operator delete shall free ptr by calling gballoc_hl_free. ]*/
        gballoc_hl_free(ptr);
    }
}

void operator delete[](void* ptr) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_026: [ operator delete[] shall call operator delete with ptr. ]*/
    ::operator delete(ptr);
}

void operator delete(void* ptr, size_t) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_027: [ Sized operator delete shall call operator delete with ptr. ]*/
    ::operator delete(ptr);
}

void operator delete[](void* ptr, size_t) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_028: [ Sized operator delete[] shall call operator delete with ptr. ]*/
    ::operator delete(ptr);
}
