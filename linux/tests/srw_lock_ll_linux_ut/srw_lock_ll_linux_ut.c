// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <pthread.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#define ENABLE_MOCKS
#include "umock_c/umock_c_prod.h"

MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_init, pthread_rwlock_t *restrict, rwlock, const pthread_rwlockattr_t *restrict, attr);
MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_wrlock, pthread_rwlock_t *, rwlock);
MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_trywrlock, pthread_rwlock_t *, rwlock);
MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_unlock, pthread_rwlock_t *, rwlock);
MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_rdlock, pthread_rwlock_t *, rwlock);
MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_tryrdlock, pthread_rwlock_t *, rwlock);
MOCKABLE_FUNCTION(, int, mocked_pthread_rwlock_destroy, pthread_rwlock_t *, rwlock);

#undef ENABLE_MOCKS

#include "c_pal/srw_lock_ll.h"

TEST_DEFINE_ENUM_TYPE(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_RESULT_VALUES)

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)
static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init");

    REGISTER_UMOCK_ALIAS_TYPE(pthread_rwlock_t*restrict, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const pthread_rwlockattr_t*restrict, void*);

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_rwlock_init, 1);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_pthread_rwlock_tryrdlock, 0, 10);
    REGISTER_GLOBAL_MOCK_RETURNS(mocked_pthread_rwlock_trywrlock, 0, 10);
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

// Tests_SRS_SRW_LOCK_LL_11_001: [ If srw_lock_ll is NULL, srw_lock_ll_init shall fail and return a non-zero value. ]
TEST_FUNCTION(srw_lock_ll_init_with_NULL_srw_lock_ll_fails)
{
    ///arrange

    ///act
    int result = srw_lock_ll_init(NULL);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_002: [ Otherwise, srw_lock_ll_init shall call InitializeSRWLock. ]
// Tests_SRS_SRW_LOCK_LL_11_003: [ otherwise, srw_lock_ll_init shall succeed and return 0. ]
TEST_FUNCTION(srw_lock_ll_init_succeeds)
{
    ///arrange
    SRW_LOCK_LL lock;

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_init(&lock, NULL));

    ///act
    int result = srw_lock_ll_init(&lock);

    ///assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_022: [ If pthread_rwlock_init returns a non-zero value, srw_lock_ll_init shall fail and return a non-zero value. ]
TEST_FUNCTION(srw_lock_ll_init_fail)
{
    ///arrange
    SRW_LOCK_LL lock;

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_init(&lock, NULL))
        .SetReturn(MU_FAILURE);

    ///act
    int result = srw_lock_ll_init(&lock);

    ///assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_deinit */

// Tests_SRS_SRW_LOCK_LL_11_004: [ If srw_lock_ll is NULL then srw_lock_ll_deinit shall return. ]
TEST_FUNCTION(srw_lock_ll_deinit_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_deinit(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_005: [ Otherwise, srw_lock_ll_deinit shall return. ]
TEST_FUNCTION(srw_lock_ll_deinit_returns)
{
    ///arrange
    SRW_LOCK_LL lock;

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_destroy(&lock));

    ///act
    srw_lock_ll_deinit(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_005: [ Otherwise, srw_lock_ll_deinit shall return. ]
TEST_FUNCTION(srw_lock_ll_deinit_on_an_initialized_lock_returns)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_destroy(&lock));

    ///act
    srw_lock_ll_deinit(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_acquire_exclusive */

// Tests_SRS_SRW_LOCK_LL_11_006: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_exclusive shall return. ]
TEST_FUNCTION(srw_lock_ll_acquire_exclusive_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_acquire_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_007: [ srw_lock_ll_acquire_exclusive shall call AcquireSRWLockExclusive. ]
TEST_FUNCTION(srw_lock_ll_acquire_exclusive_acquires_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_wrlock(&lock));

    ///act
    srw_lock_ll_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_try_acquire_exclusive */

// Tests_SRS_SRW_LOCK_LL_11_008: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_exclusive shall fail and return SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS. ]
TEST_FUNCTION(srw_lock_ll_try_acquire_exclusive_with_NULL_srw_lock_ll_fails)
{
    ///arrange

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_009: [ Otherwise srw_lock_ll_try_acquire_exclusive shall call pthread_rwlock_wrlock. ]
// Tests_SRS_SRW_LOCK_LL_11_010: [ If pthread_rwlock_trywrlock returns non-zero, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ]
TEST_FUNCTION(srw_lock_ll_try_acquire_exclusive_tries_to_acquire_lock_and_succeeds)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_trywrlock(&lock));

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_011: [ If pthread_rwlock_trywrlock returns a non-zero value, srw_lock_ll_try_acquire_exclusive shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]
TEST_FUNCTION(srw_lock_ll_try_acquire_exclusive_tries_to_acquire_lock_and_fails)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_trywrlock(&lock))
        .SetReturn(1);

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_release_exclusive */

// Tests_SRS_SRW_LOCK_LL_11_012: [ If srw_lock_ll is NULL then srw_lock_ll_release_exclusive shall return. ]
TEST_FUNCTION(srw_lock_ll_release_exclusive_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_release_exclusive(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_013: [ srw_lock_ll_release_exclusive shall call pthread_rwlock_unlock. ]
TEST_FUNCTION(srw_lock_ll_release_exclusive_releases_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    srw_lock_ll_acquire_exclusive(&lock); // just for kicks
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_unlock(&lock));

    ///act
    srw_lock_ll_release_exclusive(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_acquire_shared */

// Tests_SRS_SRW_LOCK_LL_11_014: [ If srw_lock_ll is NULL then srw_lock_ll_acquire_shared shall return. ]
TEST_FUNCTION(srw_lock_ll_acquire_shared_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_acquire_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_015: [ srw_lock_ll_acquire_shared shall call pthread_rwlock_rdlock. ]
TEST_FUNCTION(srw_lock_ll_acquire_shared_acquires_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_rdlock(&lock));

    ///act
    srw_lock_ll_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_try_acquire_shared */

// Tests_SRS_SRW_LOCK_LL_11_016: [ If srw_lock_ll is NULL then srw_lock_ll_try_acquire_shared shall fail and return SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS. ]
TEST_FUNCTION(srw_lock_ll_try_acquire_shared_with_NULL_srw_lock_ll_fails)
{
    ///arrange

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_017: [ Otherwise srw_lock_ll_try_acquire_shared shall call pthread_rwlock_tryrdlock. ]
// Tests_SRS_SRW_LOCK_LL_11_019: [ If pthread_rwlock_tryrdlock returns TRUE, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_OK. ]
TEST_FUNCTION(srw_lock_ll_try_acquire_shared_tries_to_acquire_lock_and_succeeds)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_tryrdlock(&lock));

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_018: [ If pthread_rwlock_tryrdlock returns a non-zero value, srw_lock_ll_try_acquire_shared shall return SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]
TEST_FUNCTION(srw_lock_ll_try_acquire_shared_tries_to_acquire_lock_and_fails)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_tryrdlock(&lock))
        .SetReturn(MU_FAILURE);

    ///act
    SRW_LOCK_LL_TRY_ACQUIRE_RESULT result = srw_lock_ll_try_acquire_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(SRW_LOCK_LL_TRY_ACQUIRE_RESULT, SRW_LOCK_LL_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* srw_lock_ll_release_shared */

// Tests_SRS_SRW_LOCK_LL_11_020: [ If srw_lock_ll is NULL then srw_lock_ll_release_shared shall return. ]
TEST_FUNCTION(srw_lock_ll_release_shared_with_NULL_srw_lock_ll_returns)
{
    ///arrange

    ///act
    srw_lock_ll_release_shared(NULL);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

// Tests_SRS_SRW_LOCK_LL_11_021: [ srw_lock_ll_release_shared shall call pthread_rwlock_unlock. ]
TEST_FUNCTION(srw_lock_ll_release_shared_releases_lock)
{
    ///arrange
    SRW_LOCK_LL lock;
    ASSERT_ARE_EQUAL(int, 0, srw_lock_ll_init(&lock));
    srw_lock_ll_acquire_shared(&lock); // just for kicks
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_unlock(&lock));

    ///act
    srw_lock_ll_release_shared(&lock);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
