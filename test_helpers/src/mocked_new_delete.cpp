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

// Mocking is enabled while the lock exists.
static SRW_LOCK_HANDLE g_lock = nullptr;

int mocked_new_delete_enable(void)
{
    int result;

    if (g_lock != nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_001: [ If mocking is already enabled, mocked_new_delete_enable shall return 0 without creating another lock. ]*/
        result = 0;
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_002: [ Otherwise, mocked_new_delete_enable shall: ]*/
        /*Codes_SRS_MOCKED_NEW_DELETE_16_003: [ Create the lock by calling srw_lock_create with false and an empty lock name. ]*/
        SRW_LOCK_HANDLE lock = srw_lock_create(false, "");
        if (lock == nullptr)
        {
            /*Codes_SRS_MOCKED_NEW_DELETE_16_004: [ If srw_lock_create fails, leave mocking disabled and return a non-zero value. ]*/
            LogError("srw_lock_create failed");
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_MOCKED_NEW_DELETE_16_005: [ Otherwise, enable mocking and return 0. ]*/
            g_lock = lock;
            result = 0;
        }
    }

    return result;
}

void mocked_new_delete_disable(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_006: [ If mocking is not enabled, mocked_new_delete_disable shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_007: [ Otherwise, mocked_new_delete_disable shall: ]*/
        SRW_LOCK_HANDLE lock = g_lock;
        /*Codes_SRS_MOCKED_NEW_DELETE_16_008: [ Disable mocking. ]*/
        g_lock = nullptr;
        /*Codes_SRS_MOCKED_NEW_DELETE_16_009: [ Destroy the lock by calling srw_lock_destroy. ]*/
        srw_lock_destroy(lock);
    }
}

void mocked_new_delete_lock(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_010: [ If mocking is not enabled, mocked_new_delete_lock shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_011: [ Otherwise, mocked_new_delete_lock shall acquire the lock by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(g_lock);
    }
}

void mocked_new_delete_unlock(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_012: [ If mocking is not enabled, mocked_new_delete_unlock shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_013: [ Otherwise, mocked_new_delete_unlock shall release the lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(g_lock);
    }
}

// The operators below share this translation unit with mocked_new_delete_enable, so a binary that calls it
// always links these replacements too; it cannot get the control functions without them.

void* operator new(size_t size)
{
    void* result;

    /*Codes_SRS_MOCKED_NEW_DELETE_16_014: [ If size is 0, operator new shall allocate 1 byte instead. ]*/
    size_t allocation_size = (size == 0) ? 1 : size;

    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_015: [ If mocking is not enabled, operator new shall allocate memory by calling gballoc_hl_malloc with the size to allocate. ]*/
        result = gballoc_hl_malloc(allocation_size);
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_016: [ Otherwise, operator new shall: ]*/
        /*Codes_SRS_MOCKED_NEW_DELETE_16_017: [ Acquire the lock by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(g_lock);
        /*Codes_SRS_MOCKED_NEW_DELETE_16_018: [ Allocate memory by calling mocked_new_delete_malloc with the size to allocate. ]*/
        result = mocked_new_delete_malloc(allocation_size);
        /*Codes_SRS_MOCKED_NEW_DELETE_16_019: [ Release the lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(g_lock);
    }

    if (result == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_020: [ If allocating memory fails, operator new shall throw std::bad_alloc. ]*/
        throw std::bad_alloc();
    }
    else
    {
        /* do nothing */
    }

    /*Codes_SRS_MOCKED_NEW_DELETE_16_021: [ Otherwise, operator new shall return the allocated memory. ]*/
    return result;
}

void* operator new[](size_t size)
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_022: [ operator new[] shall return the result of calling operator new with size, propagating std::bad_alloc. ]*/
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept
{
    if (ptr == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_023: [ If ptr is NULL, operator delete shall return. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_024: [ Otherwise, operator delete shall free ptr by calling gballoc_hl_free, even if mocking is enabled. ]*/
        gballoc_hl_free(ptr);
    }
}

void operator delete[](void* ptr) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_025: [ operator delete[] shall call operator delete with ptr. ]*/
    ::operator delete(ptr);
}

void operator delete(void* ptr, size_t) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_026: [ Sized operator delete shall call operator delete with ptr. ]*/
    ::operator delete(ptr);
}

void operator delete[](void* ptr, size_t) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_027: [ Sized operator delete[] shall call operator delete with ptr. ]*/
    ::operator delete(ptr);
}
