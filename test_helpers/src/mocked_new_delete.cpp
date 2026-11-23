// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstddef>
#include <new>

#include "c_pal/gballoc_hl_redirect.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/srw_lock.h"

#include "c_pal/mocked_new_delete.h"

static SRW_LOCK_HANDLE g_lock = nullptr;

int use_mocks_for_new_delete(void)
{
    int result;

    if (g_lock != nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_004: [ If mocking is already enabled, use_mocks_for_new_delete shall leave the existing lock unchanged and return 0. ]*/
        result = 0;
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_022: [ Otherwise, use_mocks_for_new_delete shall: ]*/
        /*Codes_SRS_MOCKED_NEW_DELETE_16_001: [ Create an exclusive lock by calling srw_lock_create with false and an empty lock name. ]*/
        SRW_LOCK_HANDLE lock = srw_lock_create(false, "");
        if (lock == nullptr)
        {
            /*Codes_SRS_MOCKED_NEW_DELETE_16_002: [ If srw_lock_create fails, use_mocks_for_new_delete shall leave mocking disabled and return a non-zero value. ]*/
            LogError("srw_lock_create failed");
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_MOCKED_NEW_DELETE_16_029: [ Otherwise, use_mocks_for_new_delete shall: ]*/
            /*Codes_SRS_MOCKED_NEW_DELETE_16_028: [ Enable mocking. ]*/
            g_lock = lock;
            /*Codes_SRS_MOCKED_NEW_DELETE_16_003: [ Return 0 on success. ]*/
            result = 0;
        }
    }

    return result;
}

void use_default_malloc_for_new_delete(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_006: [ If mocking is not enabled, use_default_malloc_for_new_delete shall do nothing. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_005: [ Otherwise, use_default_malloc_for_new_delete shall: ]*/
        SRW_LOCK_HANDLE lock = g_lock;
        /*Codes_SRS_MOCKED_NEW_DELETE_16_023: [ Disable mocking. ]*/
        g_lock = nullptr;
        /*Codes_SRS_MOCKED_NEW_DELETE_16_024: [ Destroy the exclusive lock by calling srw_lock_destroy. ]*/
        srw_lock_destroy(lock);
    }
}

void lock_mocked_malloc(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_008: [ If mocking is not enabled, lock_mocked_malloc shall do nothing. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_007: [ Otherwise, lock_mocked_malloc shall acquire the exclusive lock by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(g_lock);
    }
}

void unlock_mocked_malloc(void)
{
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_010: [ If mocking is not enabled, unlock_mocked_malloc shall do nothing. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_009: [ Otherwise, unlock_mocked_malloc shall release the exclusive lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(g_lock);
    }
}

void* operator new(size_t size)
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_011: [ If size is 0, operator new shall request 1 byte instead. ]*/
    if (size == 0)
    {
        size = 1;
    }
    else
    {
        /* do nothing */
    }

    void* result;
    if (g_lock == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_012: [ If mocking is disabled, operator new shall allocate memory by calling gballoc_hl_malloc. ]*/
        result = gballoc_hl_malloc(size);
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_013: [ Otherwise, operator new shall: ]*/
        /*Codes_SRS_MOCKED_NEW_DELETE_16_025: [ Acquire the exclusive lock by calling srw_lock_acquire_exclusive. ]*/
        srw_lock_acquire_exclusive(g_lock);
        /*Codes_SRS_MOCKED_NEW_DELETE_16_026: [ Allocate memory by calling mocked_malloc with the requested size. ]*/
        result = mocked_malloc(size);
        /*Codes_SRS_MOCKED_NEW_DELETE_16_027: [ Release the exclusive lock by calling srw_lock_release_exclusive. ]*/
        srw_lock_release_exclusive(g_lock);
    }

    if (result == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_014: [ If allocation fails, operator new shall throw std::bad_alloc. ]*/
        throw std::bad_alloc();
    }
    else
    {
        /* do nothing */
    }

    /*Codes_SRS_MOCKED_NEW_DELETE_16_015: [ Otherwise, operator new shall return the allocated memory on success. ]*/
    return result;
}

void* operator new[](size_t size)
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_016: [ operator new[] shall allocate memory by calling operator new with size, propagating its result or exception. ]*/
    return ::operator new(size);
}

void operator delete(void* ptr) noexcept
{
    if (ptr == nullptr)
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_017: [ If ptr is NULL, operator delete(void* ptr) shall do nothing. ]*/
    }
    else
    {
        /*Codes_SRS_MOCKED_NEW_DELETE_16_018: [ Otherwise, operator delete(void* ptr) shall release ptr by calling gballoc_hl_free, regardless of whether mocking is enabled. ]*/
        gballoc_hl_free(ptr);
    }
}

void operator delete[](void* ptr) noexcept
{
    /*Codes_SRS_MOCKED_NEW_DELETE_16_019: [ operator delete[](void* ptr) shall release ptr by calling operator delete(void* ptr). ]*/
    ::operator delete(ptr);
}

void operator delete(void* ptr, size_t size) noexcept
{
    (void)size;
    /*Codes_SRS_MOCKED_NEW_DELETE_16_020: [ operator delete(void* ptr, size_t size) shall release ptr by calling operator delete(void* ptr). ]*/
    ::operator delete(ptr);
}

void operator delete[](void* ptr, size_t size) noexcept
{
    (void)size;
    /*Codes_SRS_MOCKED_NEW_DELETE_16_021: [ operator delete[](void* ptr, size_t size) shall release ptr by calling operator delete(void* ptr). ]*/
    ::operator delete(ptr);
}
