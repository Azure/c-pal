// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstddef>
#include <new>

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"

#include "c_pal/gballoc_hl_redirect.h"

#include "macro_utils/macro_utils.h"

#include "c_pal/gballoc_hl.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h"
#include "c_pal/srw_lock.h"
#include "c_pal/mocked_new_delete.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h"

#include "real_gballoc_hl.h"
#include "real_srw_lock.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static SRW_LOCK_HANDLE g_test_lock;

static void internal_on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c error=%s", MU_ENUM_TO_STRING(UMOCK_C_ERROR_CODE, error_code));
}

static void internal_enable_mocking(void)
{
    STRICT_EXPECTED_CALL(srw_lock_create(false, ""))
        .CaptureReturn(&g_test_lock);

    ASSERT_ARE_EQUAL(int, 0, use_mocks_for_new_delete());
    ASSERT_IS_NOT_NULL(g_test_lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

static void internal_expect_allocation(size_t size)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));
    STRICT_EXPECTED_CALL(mocked_malloc(size));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));
}

static void internal_destroy_lock_with_allocation(SRW_LOCK_HANDLE lock)
{
    void* allocation = ::operator new(32);
    ASSERT_IS_NOT_NULL(allocation);
    ::operator delete(allocation);
    real_srw_lock_destroy(lock);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(nullptr, nullptr));
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(nullptr, nullptr));
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(internal_on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());

    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_create, nullptr);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_malloc, gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_malloc, nullptr);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    use_default_malloc_for_new_delete();
    umock_c_deinit();
    real_gballoc_hl_deinit();
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(test_init)
{
    g_test_lock = nullptr;
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(test_cleanup)
{
    use_default_malloc_for_new_delete();
    umock_c_reset_all_calls();
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_001: [ If mocking is not enabled, use_mocks_for_new_delete shall create an exclusive lock by calling srw_lock_create with false and an empty lock name. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_003: [ use_mocks_for_new_delete shall enable mocking and return 0 on success. ]*/
TEST_FUNCTION(when_enabling_mocking_then_the_lock_is_created)
{
    // arrange
    STRICT_EXPECTED_CALL(srw_lock_create(false, ""))
        .CaptureReturn(&g_test_lock);

    // act
    int result = use_mocks_for_new_delete();

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_IS_NOT_NULL(g_test_lock);
    internal_expect_allocation(32);
    void* allocation = ::operator new(32);
    ASSERT_IS_NOT_NULL(allocation);
    ::operator delete(allocation);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_002: [ If srw_lock_create fails, use_mocks_for_new_delete shall leave mocking disabled and return a non-zero value. ]*/
TEST_FUNCTION(when_lock_creation_fails_then_mocking_stays_disabled)
{
    // arrange
    STRICT_EXPECTED_CALL(srw_lock_create(false, ""))
        .SetReturn(nullptr);

    // act
    int result = use_mocks_for_new_delete();

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    void* allocation = ::operator new(32);
    ASSERT_IS_NOT_NULL(allocation);
    ::operator delete(allocation);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_004: [ If mocking is already enabled, use_mocks_for_new_delete shall leave the existing lock unchanged and return 0. ]*/
TEST_FUNCTION(when_enabling_mocking_twice_then_the_lock_is_reused)
{
    // arrange
    internal_enable_mocking();

    // act
    int result = use_mocks_for_new_delete();

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));
    lock_mocked_malloc();
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));
    unlock_mocked_malloc();
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_005: [ If mocking is enabled, use_default_malloc_for_new_delete shall disable mocking before destroying the exclusive lock by calling srw_lock_destroy. ]*/
TEST_FUNCTION(when_disabling_mocking_then_the_lock_is_destroyed_and_allocations_bypass_mocks)
{
    // arrange
    internal_enable_mocking();
    STRICT_EXPECTED_CALL(srw_lock_destroy(g_test_lock));
    REGISTER_GLOBAL_MOCK_HOOK(srw_lock_destroy, internal_destroy_lock_with_allocation);

    // act
    use_default_malloc_for_new_delete();

    // assert
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    void* allocation = ::operator new(32);
    ASSERT_IS_NOT_NULL(allocation);
    ::operator delete(allocation);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_006: [ If mocking is not enabled, use_default_malloc_for_new_delete shall do nothing. ]*/
TEST_FUNCTION(when_mocking_is_disabled_then_disabling_it_again_does_nothing)
{
    // arrange

    // act
    use_default_malloc_for_new_delete();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_007: [ If mocking is enabled, lock_mocked_malloc shall acquire the exclusive lock by calling srw_lock_acquire_exclusive. ]*/
TEST_FUNCTION(when_locking_mocked_malloc_then_the_lock_is_acquired)
{
    // arrange
    internal_enable_mocking();
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));

    // act
    lock_mocked_malloc();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));
    unlock_mocked_malloc();
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_008: [ If mocking is not enabled, lock_mocked_malloc shall do nothing. ]*/
TEST_FUNCTION(when_mocking_is_disabled_then_locking_does_nothing)
{
    // arrange

    // act
    lock_mocked_malloc();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_009: [ If mocking is enabled, unlock_mocked_malloc shall release the exclusive lock by calling srw_lock_release_exclusive. ]*/
TEST_FUNCTION(when_unlocking_mocked_malloc_then_the_lock_is_released)
{
    // arrange
    internal_enable_mocking();
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));
    lock_mocked_malloc();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));

    // act
    unlock_mocked_malloc();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_010: [ If mocking is not enabled, unlock_mocked_malloc shall do nothing. ]*/
TEST_FUNCTION(when_mocking_is_disabled_then_unlocking_does_nothing)
{
    // arrange

    // act
    unlock_mocked_malloc();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_012: [ If mocking is disabled, operator new shall allocate memory by calling gballoc_hl_malloc. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_015: [ operator new shall return the allocated memory on success. ]*/
TEST_FUNCTION(when_mocking_is_disabled_then_operator_new_allocates_without_mocks)
{
    // arrange

    // act
    void* result = ::operator new(32);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ::operator delete(result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_013: [ If mocking is enabled, operator new shall acquire the exclusive lock, call mocked_malloc with the requested size, and release the exclusive lock. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_015: [ operator new shall return the allocated memory on success. ]*/
TEST_FUNCTION(when_mocking_is_enabled_then_operator_new_uses_mocked_malloc)
{
    // arrange
    internal_enable_mocking();
    void* allocation = gballoc_hl_malloc(32);
    ASSERT_IS_NOT_NULL(allocation);
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));
    STRICT_EXPECTED_CALL(mocked_malloc(32))
        .SetReturn(allocation);
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));

    // act
    void* result = ::operator new(32);

    // assert
    ASSERT_ARE_EQUAL(void_ptr, allocation, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ::operator delete(result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_014: [ If allocation fails, operator new shall throw std::bad_alloc. ]*/
TEST_FUNCTION(when_mocked_malloc_fails_then_operator_new_throws_bad_alloc)
{
    // arrange
    internal_enable_mocking();
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));
    STRICT_EXPECTED_CALL(mocked_malloc(32))
        .SetReturn(nullptr);
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));
    bool caught_bad_alloc = false;

    // act
    try
    {
        void* result = ::operator new(32);
        ::operator delete(result);
    }
    catch (const std::bad_alloc&)
    {
        caught_bad_alloc = true;
    }

    // assert
    ASSERT_ARE_EQUAL(bool, true, caught_bad_alloc);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_011: [ If size is 0, operator new shall request 1 byte instead. ]*/
TEST_FUNCTION(when_operator_new_requests_zero_bytes_then_one_byte_is_allocated)
{
    // arrange
    internal_enable_mocking();
    internal_expect_allocation(1);

    // act
    void* result = ::operator new(0);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ::operator delete(result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_016: [ operator new[] shall allocate memory by calling operator new with size, propagating its result or exception. ]*/
PARAMETERIZED_TEST_FUNCTION(when_operator_new_array_is_called_then_the_request_is_forwarded,
    ARGS(size_t, size),
    CASE((0), zero_bytes),
    CASE((32), nonzero_bytes))
{
    // arrange
    internal_enable_mocking();
    internal_expect_allocation(size == 0 ? 1 : size);

    // act
    void* result = ::operator new[](size);

    // assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ::operator delete[](result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_016: [ operator new[] shall allocate memory by calling operator new with size, propagating its result or exception. ]*/
TEST_FUNCTION(when_mocked_malloc_fails_then_operator_new_array_throws_bad_alloc)
{
    // arrange
    internal_enable_mocking();
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(g_test_lock));
    STRICT_EXPECTED_CALL(mocked_malloc(32))
        .SetReturn(nullptr);
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(g_test_lock));
    bool caught_bad_alloc = false;

    // act
    try
    {
        void* result = ::operator new[](32);
        ::operator delete[](result);
    }
    catch (const std::bad_alloc&)
    {
        caught_bad_alloc = true;
    }

    // assert
    ASSERT_ARE_EQUAL(bool, true, caught_bad_alloc);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_017: [ If ptr is NULL, operator delete(void* ptr) shall do nothing. ]*/
TEST_FUNCTION(when_operator_delete_receives_null_then_it_does_nothing)
{
    // arrange

    // act
    ::operator delete(nullptr);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_018: [ If ptr is not NULL, operator delete(void* ptr) shall release ptr by calling gballoc_hl_free, regardless of whether mocking is enabled. ]*/
PARAMETERIZED_TEST_FUNCTION(when_operator_delete_is_called_then_it_bypasses_mocks,
    ARGS(bool, disable_before_delete),
    CASE((false), while_mocking),
    CASE((true), after_mocking))
{
    // arrange
    internal_enable_mocking();
    internal_expect_allocation(32);
    void* allocation = ::operator new(32);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    if (disable_before_delete)
    {
        STRICT_EXPECTED_CALL(srw_lock_destroy(g_test_lock));
        use_default_malloc_for_new_delete();
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        umock_c_reset_all_calls();
    }
    else
    {
        /* do nothing */
    }

    // act
    ::operator delete(allocation);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_019: [ operator delete[](void* ptr) shall release ptr by calling operator delete(void* ptr). ]*/
TEST_FUNCTION(when_operator_delete_array_is_called_then_it_bypasses_mocks)
{
    // arrange
    void* allocation = ::operator new[](32);
    internal_enable_mocking();

    // act
    ::operator delete[](allocation);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_020: [ operator delete(void* ptr, size_t size) shall release ptr by calling operator delete(void* ptr). ]*/
TEST_FUNCTION(when_sized_operator_delete_is_called_then_it_bypasses_mocks)
{
    // arrange
    void* allocation = ::operator new(32);
    internal_enable_mocking();

    // act
    ::operator delete(allocation, (size_t)32);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_021: [ operator delete[](void* ptr, size_t size) shall release ptr by calling operator delete(void* ptr). ]*/
TEST_FUNCTION(when_sized_operator_delete_array_is_called_then_it_bypasses_mocks)
{
    // arrange
    void* allocation = ::operator new[](32);
    internal_enable_mocking();

    // act
    ::operator delete[](allocation, (size_t)32);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
