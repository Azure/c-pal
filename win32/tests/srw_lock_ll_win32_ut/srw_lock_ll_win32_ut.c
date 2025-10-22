// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "srw_lock_ll_win32_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
MOCKABLE_FUNCTION(, void, mocked_InitializeSRWLock, PSRWLOCK, SRWLock);

MOCKABLE_FUNCTION(, void, mocked_AcquireSRWLockExclusive, PSRWLOCK, SRWLock);

MOCKABLE_FUNCTION(, BOOLEAN, mocked_TryAcquireSRWLockExclusive, PSRWLOCK, SRWLock);

MOCKABLE_FUNCTION(, void, mocked_ReleaseSRWLockExclusive, PSRWLOCK, SRWLock);

MOCKABLE_FUNCTION(, void, mocked_AcquireSRWLockShared, PSRWLOCK, SRWLock);

MOCKABLE_FUNCTION(, BOOLEAN, mocked_TryAcquireSRWLockShared, PSRWLOCK, SRWLock);

MOCKABLE_FUNCTION(, void, mocked_ReleaseSRWLockShared, PSRWLOCK, SRWLock);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types(), "umocktypes_windows_register_types");

    REGISTER_GLOBAL_MOCK_RETURNS(mocked_TryAcquireSRWLockExclusive, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_TryAcquireSRWLockShared, TRUE, FALSE);

    REGISTER_UMOCK_ALIAS_TYPE(PSRWLOCK, void*);
    
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* srw_lock_ll_init */

/* Tests_SRS_SRW_LOCK_LL_01_001: [ If srw_lock_ll is NULL, srw_lock_ll_init shall fail and return a non-zero value. ] */
TEST_FUNCTION(srw_lock_ll_init_with_NULL_srw_lock_ll_fails)
{
    ///arrange

    ///act
    int result = srw_lock_ll_init(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_002: [ Otherwise, srw_lock_ll_init shall call InitializeSRWLock. ] */
/* Tests_SRS_SRW_LOCK_LL_01_003: [ srw_lock_ll_init shall succeed and return 0. ] */
TEST_FUNCTION(srw_lock_ll_init_succeeds)
{
    ///arrange
    SRW_LOCK_LL lock;

    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));

    ///act
    int result = srw_lock_ll_init(&lock);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_deinit */

/* Tests_SRS_SRW_LOCK_LL_01_004: [ If srw_lock_ll is NULL then srw_lock_ll_deinit shall return. ] */
TEST_FUNCTION(srw_lock_ll_deinit_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_deinit(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_005: [ Otherwise, srw_lock_ll_deinit shall return. ] */
TEST_FUNCTION(srw_lock_ll_deinit_returns)
{
    ///arrange
    SRW_LOCK_LL lock;

    ///act
    srw_lock_ll_deinit(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_005: [ Otherwise, srw_lock_ll_deinit shall return. ] */
TEST_FUNCTION(srw_lock_ll_deinit_on_an_initialized_lock_returns)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    ///act
    srw_lock_ll_deinit(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_acquire_exclusive */

/* Tests_SRS_SRW_LOCK_LL_01_006: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_exclusive shall return. ] */
TEST_FUNCTION(srw_lock_ll_acquire_exclusive_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_acquire_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_007: [ srw_lock_ll_acquire_exclusive shall call AcquireSRWLockExclusive. ] */
TEST_FUNCTION(srw_lock_ll_acquire_exclusive_acquires_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockExclusive(&lock));

    ///act
    srw_lock_ll_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_try_acquire_exclusive */

/* Tests_SRS_SRW_LOCK_LL_01_008: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_exclusive shall fail and return SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS. ] */
TEST_FUNCTION(srw_lock_ll_try_acquire_exclusive_with_NULL_srw_lock_ll_fails)
{
    ///arrange

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_009: [ Otherwise srw_lock_ll_try_acquire_exclusive shall call TryAcquireSRWLockExclusive. ] */
/* Tests_SRS_SRW_LOCK_LL_01_010: [ If TryAcquireSRWLockExclusive returns TRUE, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ] */
TEST_FUNCTION(srw_lock_ll_try_acquire_exclusive_tries_to_acquire_lock_and_succeeds)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockExclusive(&lock));

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_011: [ If TryAcquireSRWLockExclusive returns FALSE, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ] */
TEST_FUNCTION(srw_lock_ll_try_acquire_exclusive_tries_to_acquire_lock_and_fails)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockExclusive(&lock))
        .SetReturn(FALSE);

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_release_exclusive */

/* Tests_SRS_SRW_LOCK_LL_01_012: [ If srw_lock_ll is NULL then srw_lock_ll_release_exclusive shall return. ] */
TEST_FUNCTION(srw_lock_ll_release_exclusive_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_release_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_013: [ srw_lock_ll_release_exclusive shall call ReleaseSRWLockExclusive. ] */
TEST_FUNCTION(srw_lock_ll_release_exclusive_releases_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    srw_lock_ll_acquire_exclusive(&lock); // just for kicks
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_ReleaseSRWLockExclusive(&lock));

    ///act
    srw_lock_ll_release_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_acquire_shared */

/* Tests_SRS_SRW_LOCK_LL_01_014: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_shared shall return. ] */
TEST_FUNCTION(srw_lock_ll_acquire_shared_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_acquire_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_015: [ srw_lock_ll_acquire_shared shall call AcquireSRWLockShared. ] */
TEST_FUNCTION(srw_lock_ll_acquire_shared_acquires_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockShared(&lock));

    ///act
    srw_lock_ll_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_try_acquire_shared */

/* Tests_SRS_SRW_LOCK_LL_01_016: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_shared shall fail and return SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS. ] */
TEST_FUNCTION(srw_lock_ll_try_acquire_shared_with_NULL_srw_lock_ll_fails)
{
    ///arrange

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_017: [ Otherwise srw_lock_ll_try_acquire_shared shall call TryAcquireSRWLockShared. ] */
/* Tests_SRS_SRW_LOCK_LL_01_019: [ If TryAcquireSRWLockShared returns TRUE, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ] */
TEST_FUNCTION(srw_lock_ll_try_acquire_shared_tries_to_acquire_lock_and_succeeds)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockShared(&lock));

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_018: [ If TryAcquireSRWLockShared returns FALSE, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ] */
TEST_FUNCTION(srw_lock_ll_try_acquire_shared_tries_to_acquire_lock_and_fails)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockShared(&lock))
        .SetReturn(FALSE);

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_release_shared */

/* Tests_SRS_SRW_LOCK_LL_01_020: [ If srw_lock_ll is NULL then srw_lock_ll_release_shared shall return. ] */
TEST_FUNCTION(srw_lock_ll_release_shared_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_release_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LL_01_021: [ srw_lock_ll_release_shared shall call ReleaseSRWLockShared. ] */
TEST_FUNCTION(srw_lock_ll_release_shared_releases_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(&lock));
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    srw_lock_ll_acquire_shared(&lock); // just for kicks
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_ReleaseSRWLockShared(&lock));

    ///act
    srw_lock_ll_release_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)