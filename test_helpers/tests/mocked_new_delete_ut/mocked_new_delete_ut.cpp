// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <cstddef>
#include <new>

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_bool.h"
#include "umock_c/umocktypes_charptr.h"

#include "c_pal/gballoc_hl_redirect.h"

#include "macro_utils/macro_utils.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "c_pal/gballoc_hl.h"
#include "c_pal/srw_lock.h"
#include "c_pal_test_helpers/mocked_new_delete.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

#include "real_gballoc_hl.h"
#include "real_srw_lock.h"

#define TEST_ALLOCATION_SIZE ((size_t)32)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static SRW_LOCK_HANDLE test_lock;
static bool allocate_in_srw_lock_destroy;

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void hook_srw_lock_destroy(SRW_LOCK_HANDLE handle)
{
    if (allocate_in_srw_lock_destroy)
    {
        // Goes through gballoc_hl_malloc only if mocked_new_delete is already disabled
        ::operator delete(::operator new(TEST_ALLOCATION_SIZE));
    }
    else
    {
        /* do nothing */
    }

    real_srw_lock_destroy(handle);
}

static void enable_mocking(void)
{
    STRICT_EXPECTED_CALL(srw_lock_create(false, ""))
        .CaptureReturn(&test_lock);

    ASSERT_ARE_EQUAL(int, 0, mocked_new_delete_enable());
    ASSERT_IS_NOT_NULL(test_lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
}

static void setup_mocked_allocation_expectations(size_t size)
{
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(test_lock));
    STRICT_EXPECTED_CALL(mocked_new_delete_malloc(size));
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(test_lock));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(nullptr, nullptr));
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_bool_register_types());
    ASSERT_ARE_EQUAL(int, 0, umocktypes_charptr_register_types());

    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();
    REGISTER_SRW_LOCK_GLOBAL_MOCK_HOOK();
    REGISTER_GLOBAL_MOCK_HOOK(srw_lock_destroy, hook_srw_lock_destroy);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(srw_lock_create, nullptr);
    REGISTER_GLOBAL_MOCK_HOOK(mocked_new_delete_malloc, real_gballoc_hl_malloc);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_new_delete_malloc, nullptr);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    test_lock = nullptr;
    allocate_in_srw_lock_destroy = false;
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init());
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    mocked_new_delete_disable();
    umock_c_negative_tests_deinit();
}

/* mocked_new_delete_enable */

/*Tests_SRS_MOCKED_NEW_DELETE_16_001: [ If mocked_new_delete is already enabled, mocked_new_delete_enable shall return 0. ]*/
TEST_FUNCTION(mocked_new_delete_enable_when_already_enabled_returns_0)
{
    ///arrange
    enable_mocking();

    ///act
    int result = mocked_new_delete_enable();

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_002: [ Otherwise: ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_003: [ mocked_new_delete_enable shall create the lock by calling srw_lock_create with false and an empty lock name. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_004: [ mocked_new_delete_enable shall mark mocked_new_delete as enabled. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_005: [ mocked_new_delete_enable shall return 0 on success. ]*/
TEST_FUNCTION(mocked_new_delete_enable_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(srw_lock_create(false, ""))
        .CaptureReturn(&test_lock);

    ///act
    int result = mocked_new_delete_enable();

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // mocked_new_delete is enabled: operator new now goes through mocked_new_delete_malloc under the lock
    umock_c_reset_all_calls();
    setup_mocked_allocation_expectations(TEST_ALLOCATION_SIZE);
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));
    ::operator delete(::operator new(TEST_ALLOCATION_SIZE));
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_006: [ If there are any failures, mocked_new_delete_enable shall fail and return a non-zero value. ]*/
TEST_FUNCTION(when_srw_lock_create_fails_then_mocked_new_delete_enable_fails)
{
    ///arrange
    STRICT_EXPECTED_CALL(srw_lock_create(false, ""));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            ///act
            int result = mocked_new_delete_enable();

            ///assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result, "On failed call %zu", i);
        }
    }
}

/* mocked_new_delete_disable */

/*Tests_SRS_MOCKED_NEW_DELETE_16_007: [ If mocked_new_delete is disabled, mocked_new_delete_disable shall return. ]*/
TEST_FUNCTION(mocked_new_delete_disable_when_disabled_returns)
{
    ///arrange

    ///act
    mocked_new_delete_disable();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_008: [ Otherwise: ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_009: [ mocked_new_delete_disable shall mark mocked_new_delete as disabled. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_010: [ mocked_new_delete_disable shall destroy the lock by calling srw_lock_destroy. ]*/
TEST_FUNCTION(mocked_new_delete_disable_marks_mocked_new_delete_as_disabled_before_destroying_the_lock)
{
    ///arrange
    enable_mocking();
    allocate_in_srw_lock_destroy = true;
    STRICT_EXPECTED_CALL(srw_lock_destroy(test_lock));
    STRICT_EXPECTED_CALL(gballoc_hl_malloc(TEST_ALLOCATION_SIZE));
    STRICT_EXPECTED_CALL(gballoc_hl_free(IGNORED_ARG));

    ///act
    mocked_new_delete_disable();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* mocked_new_delete_lock */

/*Tests_SRS_MOCKED_NEW_DELETE_16_011: [ If mocked_new_delete is disabled, mocked_new_delete_lock shall return. ]*/
TEST_FUNCTION(mocked_new_delete_lock_when_disabled_returns)
{
    ///arrange

    ///act
    mocked_new_delete_lock();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_012: [ Otherwise, mocked_new_delete_lock shall acquire the lock by calling srw_lock_acquire_exclusive. ]*/
TEST_FUNCTION(mocked_new_delete_lock_acquires_the_lock)
{
    ///arrange
    enable_mocking();
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(test_lock));

    ///act
    mocked_new_delete_lock();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    mocked_new_delete_unlock();
}

/* mocked_new_delete_unlock */

/*Tests_SRS_MOCKED_NEW_DELETE_16_013: [ If mocked_new_delete is disabled, mocked_new_delete_unlock shall return. ]*/
TEST_FUNCTION(mocked_new_delete_unlock_when_disabled_returns)
{
    ///arrange

    ///act
    mocked_new_delete_unlock();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_014: [ Otherwise, mocked_new_delete_unlock shall release the lock by calling srw_lock_release_exclusive. ]*/
TEST_FUNCTION(mocked_new_delete_unlock_releases_the_lock)
{
    ///arrange
    enable_mocking();
    mocked_new_delete_lock();
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(test_lock));

    ///act
    mocked_new_delete_unlock();

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* operator new */

/*Tests_SRS_MOCKED_NEW_DELETE_16_015: [ If size is 0, operator new shall allocate 1 byte instead. ]*/
TEST_FUNCTION(operator_new_with_size_0_allocates_1_byte)
{
    ///arrange
    enable_mocking();
    setup_mocked_allocation_expectations(1);

    ///act
    void* result = ::operator new(0);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    ::operator delete(result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_016: [ If mocked_new_delete is disabled, operator new shall allocate memory by calling gballoc_hl_malloc with the size to allocate. ]*/
TEST_FUNCTION(operator_new_when_mocked_new_delete_is_disabled_calls_gballoc_hl_malloc)
{
    ///arrange
    void* allocation = nullptr;
    STRICT_EXPECTED_CALL(gballoc_hl_malloc(TEST_ALLOCATION_SIZE))
        .CaptureReturn(&allocation);

    ///act
    void* result = ::operator new(TEST_ALLOCATION_SIZE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(void_ptr, allocation, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    ::operator delete(result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_017: [ Otherwise: ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_018: [ operator new shall acquire the lock by calling srw_lock_acquire_exclusive. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_019: [ operator new shall allocate memory by calling mocked_new_delete_malloc with the size to allocate. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_020: [ operator new shall release the lock by calling srw_lock_release_exclusive. ]*/
/*Tests_SRS_MOCKED_NEW_DELETE_16_021: [ operator new shall return the allocated memory on success. ]*/
TEST_FUNCTION(operator_new_when_mocked_new_delete_is_enabled_calls_mocked_new_delete_malloc_under_the_lock)
{
    ///arrange
    enable_mocking();
    void* allocation = nullptr;
    STRICT_EXPECTED_CALL(srw_lock_acquire_exclusive(test_lock));
    STRICT_EXPECTED_CALL(mocked_new_delete_malloc(TEST_ALLOCATION_SIZE))
        .CaptureReturn(&allocation);
    STRICT_EXPECTED_CALL(srw_lock_release_exclusive(test_lock));

    ///act
    void* result = ::operator new(TEST_ALLOCATION_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(void_ptr, allocation, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    ::operator delete(result);
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_022: [ If there are any failures, operator new shall throw std::bad_alloc. ]*/
TEST_FUNCTION(when_mocked_new_delete_malloc_fails_then_operator_new_throws_bad_alloc)
{
    ///arrange
    enable_mocking();
    setup_mocked_allocation_expectations(TEST_ALLOCATION_SIZE);

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            bool caught_bad_alloc = false;

            ///act
            try
            {
                ::operator delete(::operator new(TEST_ALLOCATION_SIZE));
            }
            catch (const std::bad_alloc&)
            {
                caught_bad_alloc = true;
            }

            ///assert
            ASSERT_ARE_EQUAL(bool, true, caught_bad_alloc, "On failed call %zu", i);
            ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "On failed call %zu", i);
        }
    }
}

/* operator new[] */

/*Tests_SRS_MOCKED_NEW_DELETE_16_023: [ operator new[] shall return the result of calling operator new with size. ]*/
TEST_FUNCTION(operator_new_array_calls_operator_new)
{
    ///arrange
    enable_mocking();
    setup_mocked_allocation_expectations(TEST_ALLOCATION_SIZE);

    ///act
    void* result = ::operator new[](TEST_ALLOCATION_SIZE);

    ///assert
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///cleanup
    ::operator delete[](result);
}

/* operator delete */

/*Tests_SRS_MOCKED_NEW_DELETE_16_024: [ If ptr is NULL, operator delete shall return. ]*/
TEST_FUNCTION(operator_delete_with_NULL_returns)
{
    ///arrange

    ///act
    ::operator delete(nullptr);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_MOCKED_NEW_DELETE_16_025: [ Otherwise, operator delete shall free ptr by calling gballoc_hl_free. ]*/
TEST_FUNCTION(operator_delete_when_mocked_new_delete_is_enabled_frees_ptr_by_calling_gballoc_hl_free)
{
    ///arrange
    enable_mocking();
    void* ptr = ::operator new(TEST_ALLOCATION_SIZE);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_hl_free(ptr));

    ///act
    ::operator delete(ptr);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* operator delete[] */

/*Tests_SRS_MOCKED_NEW_DELETE_16_026: [ operator delete[] shall call operator delete with ptr. ]*/
TEST_FUNCTION(operator_delete_array_frees_ptr_by_calling_gballoc_hl_free)
{
    ///arrange
    void* ptr = ::operator new[](TEST_ALLOCATION_SIZE);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_hl_free(ptr));

    ///act
    ::operator delete[](ptr);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* sized operator delete */

/*Tests_SRS_MOCKED_NEW_DELETE_16_027: [ Sized operator delete shall call operator delete with ptr. ]*/
TEST_FUNCTION(sized_operator_delete_frees_ptr_by_calling_gballoc_hl_free)
{
    ///arrange
    void* ptr = ::operator new(TEST_ALLOCATION_SIZE);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_hl_free(ptr));

    ///act
    ::operator delete(ptr, TEST_ALLOCATION_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* sized operator delete[] */

/*Tests_SRS_MOCKED_NEW_DELETE_16_028: [ Sized operator delete[] shall call operator delete with ptr. ]*/
TEST_FUNCTION(sized_operator_delete_array_frees_ptr_by_calling_gballoc_hl_free)
{
    ///arrange
    void* ptr = ::operator new[](TEST_ALLOCATION_SIZE);
    umock_c_reset_all_calls();
    STRICT_EXPECTED_CALL(gballoc_hl_free(ptr));

    ///act
    ::operator delete[](ptr, TEST_ALLOCATION_SIZE);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
