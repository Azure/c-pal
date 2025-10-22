// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "srw_lock_win32_ut_pch.h"

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

TEST_DEFINE_ENUM_TYPE(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_RESULT_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static void my_timer_destroy(TIMER_HANDLE timer)
{
    real_gballoc_hl_free(timer);
}

static SRW_LOCK_HANDLE TEST_srw_lock_create(bool do_statistics, const char* lock_name)
{
    SRW_LOCK_HANDLE result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    if (do_statistics)
    {
        STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
        STRICT_EXPECTED_CALL(timer_create_new())
            .SetReturn((TIMER_HANDLE)real_gballoc_hl_malloc(2));
    }
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(IGNORED_ARG));
    result = srw_lock_create(do_statistics, lock_name);
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

static void TEST_srw_lock_acquire_shared(SRW_LOCK_HANDLE handle, double pretendTimeElapsed)
{
    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockShared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(pretendTimeElapsed);

    if (pretendTimeElapsed >= 600) /*10 minutes, hardcoded*/
    {
        STRICT_EXPECTED_CALL(timer_start(IGNORED_ARG));
    }

    srw_lock_acquire_shared(handle);
    umock_c_reset_all_calls();
}

static void TEST_srw_lock_acquire_exclusive(SRW_LOCK_HANDLE handle, double pretendTimeElapsed)
{
    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockExclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(pretendTimeElapsed);

    if (pretendTimeElapsed >= 600) /*10 minutes, hardcoded*/
    {
        STRICT_EXPECTED_CALL(timer_start(IGNORED_ARG));
    }

    srw_lock_acquire_exclusive(handle);
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types(), "umocktypes_windows_register_types");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_HOOK(timer_destroy, my_timer_destroy);

    REGISTER_GLOBAL_MOCK_RETURNS(mocked_TryAcquireSRWLockExclusive, TRUE, FALSE);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_TryAcquireSRWLockShared, TRUE, FALSE);

    REGISTER_UMOCK_ALIAS_TYPE(TIMER_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(PSRWLOCK, void*);

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
}

/* srw_lock_create */

/*Tests_SRS_SRW_LOCK_02_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE. ]*/
/*Tests_SRS_SRW_LOCK_02_023: [ If do_statistics is true then srw_lock_create shall copy lock_name. ]*/
/*Tests_SRS_SRW_LOCK_02_024: [ If do_statistics is true then srw_lock_create shall create a new TIMER_HANDLE by calling timer_create_new. ]*/
/*Tests_SRS_SRW_LOCK_02_015: [ srw_lock_create shall call InitializeSRWLock. ]*/
/*Tests_SRS_SRW_LOCK_02_003: [ srw_lock_create shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(srw_lock_create_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_create_new())
        .SetReturn((TIMER_HANDLE)real_gballoc_hl_malloc(2));
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(IGNORED_ARG));

    ///act
    SRW_LOCK_HANDLE bsdlLock = srw_lock_create(true, "test_lock");

    ///assert
    ASSERT_IS_NOT_NULL(bsdlLock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_destroy(bsdlLock);
}

/*Tests_SRS_SRW_LOCK_02_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE. ]*/
/*Tests_SRS_SRW_LOCK_02_023: [ If do_statistics is true then srw_lock_create shall copy lock_name. ]*/
/*Tests_SRS_SRW_LOCK_02_024: [ If do_statistics is true then srw_lock_create shall create a new TIMER_HANDLE by calling timer_create_new. ]*/
/*Tests_SRS_SRW_LOCK_02_015: [ srw_lock_create shall call InitializeSRWLock. ]*/
/*Tests_SRS_SRW_LOCK_02_003: [ srw_lock_create shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(srw_lock_create_with_do_statistics_false_succeeds)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_InitializeSRWLock(IGNORED_ARG));

    ///act
    SRW_LOCK_HANDLE bsdlLock = srw_lock_create(false, "test_lock");

    ///assert
    ASSERT_IS_NOT_NULL(bsdlLock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_destroy(bsdlLock);
}


/*Tests_SRS_SRW_LOCK_02_004: [ If there are any failures then srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_1)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_create_new())
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    SRW_LOCK_HANDLE bsdlLock = srw_lock_create(true, "test_lock");

    ///assert
    ASSERT_IS_NULL(bsdlLock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SRW_LOCK_02_004: [ If there are any failures then srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_2)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    SRW_LOCK_HANDLE bsdlLock = srw_lock_create(true, "test_lock");

    ///assert
    ASSERT_IS_NULL(bsdlLock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SRW_LOCK_02_004: [ If there are any failures then srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_3)
{
    ///arrange
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG))
        .SetReturn(NULL);

    ///act
    SRW_LOCK_HANDLE bsdlLock = srw_lock_create(true, "test_lock");

    ///assert
    ASSERT_IS_NULL(bsdlLock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/* srw_lock_acquire_exclusive */

/*Tests_SRS_SRW_LOCK_02_022: [ If handle is NULL then srw_lock_acquire_exclusive shall return. ]*/
TEST_FUNCTION(srw_lock_acquire_exclusive_with_handle_NULL_returns)
{
    ///arrange

    ///act
    srw_lock_acquire_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SRW_LOCK_02_006: [ srw_lock_acquire_exclusive shall call AcquireSRWLockExclusive. ]*/
/*Tests_SRS_SRW_LOCK_02_025: [ If do_statistics is true and if the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_acquire_exclusive_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockExclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(0);

    ///act
    srw_lock_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/*Tests_SRS_SRW_LOCK_02_006: [ srw_lock_acquire_exclusive shall call AcquireSRWLockExclusive. ]*/
/*Tests_SRS_SRW_LOCK_02_025: [ If do_statistics is true and if the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_acquire_exclusive_with_do_statistics_false_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(false, "test_lock");

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockExclusive(IGNORED_ARG));

    ///act
    srw_lock_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}


/*Tests_SRS_SRW_LOCK_02_025: [ If do_statistics is true and if the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_acquire_exclusive_restarts_timer_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockExclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(10000);
    STRICT_EXPECTED_CALL(timer_start(IGNORED_ARG));

    ///act
    srw_lock_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/* srw_lock_try_acquire_exclusive */

/* Tests_SRS_SRW_LOCK_01_006: [ If handle is NULL then srw_lock_try_acquire_exclusive shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_with_handle_NULL_returns_SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS)
{
    ///arrange

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SRW_LOCK_01_007: [ Otherwise srw_lock_acquire_exclusive shall call TryAcquireSRWLockExclusive. ]*/
/*Tests_SRS_SRW_LOCK_01_009: [ If TryAcquireSRWLockExclusive returns TRUE, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
/*Tests_SRS_SRW_LOCK_01_010: [ If do_statistics is true and if the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockExclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(0);

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/*Tests_SRS_SRW_LOCK_01_007: [ Otherwise srw_lock_acquire_exclusive shall call TryAcquireSRWLockExclusive. ]*/
/*Tests_SRS_SRW_LOCK_01_009: [ If TryAcquireSRWLockExclusive returns TRUE, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
/*Tests_SRS_SRW_LOCK_01_010: [ If do_statistics is true and if the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_with_do_statistics_false_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(false, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockExclusive(IGNORED_ARG));

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}


/*Tests_SRS_SRW_LOCK_02_025: [ If do_statistics is true and if the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_restarts_timer_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockExclusive(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(10000);
    STRICT_EXPECTED_CALL(timer_start(IGNORED_ARG));

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/* Tests_SRS_SRW_LOCK_01_008: [ If TryAcquireSRWLockExclusive returns FALSE, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]*/
TEST_FUNCTION(when_underlying_TryAcquireSRWLockExclusive_returns_FALSE_srw_lock_try_acquire_exclusive_returns_SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE_and_does_no_time_measurement)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockExclusive(IGNORED_ARG))
        .SetReturn(FALSE);

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_exclusive(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/* srw_lock_release_exclusive */

/*Tests_SRS_SRW_LOCK_02_009: [ If handle is NULL then srw_lock_release_exclusive shall return. ]*/
TEST_FUNCTION(srw_lock_release_exclusive_with_handle_NULL_returns)
{
    ///arrange

    ///act
    srw_lock_release_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
}

/*Tests_SRS_SRW_LOCK_02_010: [ srw_lock_release_exclusive shall call ReleaseSRWLockExclusive. ]*/
TEST_FUNCTION(srw_lock_release_exclusive_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");
    TEST_srw_lock_acquire_exclusive(bsdlLock, 1);


    STRICT_EXPECTED_CALL(mocked_ReleaseSRWLockExclusive(IGNORED_ARG));

    ///act
    srw_lock_release_exclusive(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_destroy(bsdlLock);
}

/* srw_lock_destroy */

/*Tests_SRS_SRW_LOCK_02_011: [ If handle is NULL then srw_lock_destroy shall return. ]*/
TEST_FUNCTION(srw_lock_destroy_with_handle_NULL_returns)
{

    ///act
    srw_lock_destroy(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SRW_LOCK_02_012: [ srw_lock_destroy shall free all used resources. ]*/
TEST_FUNCTION(srw_lock_destroy_free_used_resources)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(timer_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    ///act
    srw_lock_destroy(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_acquire_shared */

/*Tests_SRS_SRW_LOCK_02_017: [ If handle is NULL then srw_lock_acquire_shared shall return. ]*/
TEST_FUNCTION(srw_lock_acquire_shared_with_handle_NULL_returns)
{
    ///arrange

    ///act
    srw_lock_acquire_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/*Tests_SRS_SRW_LOCK_02_018: [ srw_lock_acquire_shared shall call AcquireSRWLockShared. ]*/
TEST_FUNCTION(srw_lock_acquire_shared_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockShared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(0);

    ///act
    srw_lock_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}


/*Tests_SRS_SRW_LOCK_02_026: [ If do_statistics is true and the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_acquire_shared_restarts_timer_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockShared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(10000);
    STRICT_EXPECTED_CALL(timer_start(IGNORED_ARG));

    ///act
    srw_lock_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/*Tests_SRS_SRW_LOCK_02_026: [ If do_statistics is true and the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_acquire_shared_with_do_statistic_false_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(false, "test_lock");

    STRICT_EXPECTED_CALL(mocked_AcquireSRWLockShared(IGNORED_ARG));

    ///act
    srw_lock_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/* srw_lock_try_acquire_shared */

/* Tests_SRS_SRW_LOCK_01_001: [ If handle is NULL then srw_lock_try_acquire_shared shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_with_handle_NULL_returns_SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS)
{
    ///arrange

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SRW_LOCK_01_002: [ Otherwise srw_lock_try_acquire_shared shall call TryAcquireSRWLockShared. ]*/
/*Tests_SRS_SRW_LOCK_01_004: [ If TryAcquireSRWLockShared returns TRUE, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
/*Tests_SRS_SRW_LOCK_01_005: [ If do_statistics is true and the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockShared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(0);

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/*Tests_SRS_SRW_LOCK_01_007: [ Otherwise srw_lock_acquire_shared shall call TryAcquireSRWLockShared. ]*/
/*Tests_SRS_SRW_LOCK_01_004: [ If TryAcquireSRWLockShared returns TRUE, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
/*Tests_SRS_SRW_LOCK_01_005: [ If do_statistics is true and the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_with_do_statistics_false_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(false, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockShared(IGNORED_ARG));

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}


/*Tests_SRS_SRW_LOCK_01_005: [ If do_statistics is true and the timer created has recorded more than TIME_BETWEEN_STATISTICS_LOG seconds then statistics will be logged and the timer shall be started again. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_restarts_timer_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockShared(IGNORED_ARG));
    STRICT_EXPECTED_CALL(timer_get_elapsed(IGNORED_ARG))
        .SetReturn(10000);
    STRICT_EXPECTED_CALL(timer_start(IGNORED_ARG));

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/* Tests_SRS_SRW_LOCK_01_003: [ If TryAcquireSRWLockShared returns FALSE, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]*/
TEST_FUNCTION(when_underlying_TryAcquireSRWLockShared_returns_FALSE_srw_lock_try_acquire_shared_returns_SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE_and_does_no_time_measurement)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(mocked_TryAcquireSRWLockShared(IGNORED_ARG))
        .SetReturn(FALSE);

    ///act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_release_shared(bsdlLock);
    srw_lock_destroy(bsdlLock);
}

/* srw_lock_release_shared */

/*Tests_SRS_SRW_LOCK_02_020: [ If handle is NULL then srw_lock_release_shared shall return. ]*/
TEST_FUNCTION(srw_lock_release_shared_with_handle_NULL_returns)
{
    ///act
    srw_lock_release_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_SRW_LOCK_02_021: [ srw_lock_release_shared shall call ReleaseSRWLockShared. ]*/
TEST_FUNCTION(srw_lock_release_shared_succeeds)
{
    ///arrange
    SRW_LOCK_HANDLE bsdlLock = TEST_srw_lock_create(true, "test_lock");
    TEST_srw_lock_acquire_shared(bsdlLock, 1);

    STRICT_EXPECTED_CALL(mocked_ReleaseSRWLockShared(IGNORED_ARG));

    ///act
    srw_lock_release_shared(bsdlLock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    ///clean
    srw_lock_destroy(bsdlLock);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)