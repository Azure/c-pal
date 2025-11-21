// Copyright (c) Microsoft. All rights reserved.


#include "srw_lock_linux_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
char* sprintf_char_function(const char* format, ...)
{
    char* result;
    va_list va;
    va_start(va, format);
    result = vsprintf_char(format, va);
    va_end(va);
    return result;
}
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

TEST_DEFINE_ENUM_TYPE(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_RESULT_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

static unsigned char va_list_to_string[] = "some va_list stringification";
static char* umockvalue_stringify_va_list(const void* value)
{
    (void)value;
    char* result = real_gballoc_hl_malloc(sizeof(va_list_to_string));
    ASSERT_IS_NOT_NULL(result);
    (void)memcpy(result, va_list_to_string, sizeof(va_list_to_string));
    return result;
}

static int umockvalue_are_equal_va_list(const void* left, const void* right)
{
    (void)memcmp(left, right, sizeof(va_list));
    return 0;
}

static int umockvalue_copy_va_list(void* destination, const void* source)
{
    (void)memcpy(destination, source, sizeof(va_list));
    return 0;
}

static void umockvalue_free_va_list(void* value)
{
    (void)value;
}

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_init, pthread_rwlock_t *restrict, rwlock, const pthread_rwlockattr_t *restrict, attr)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_wrlock, pthread_rwlock_t *, rwlock)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_trywrlock, pthread_rwlock_t *, rwlock)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_unlock, pthread_rwlock_t *, rwlock)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_rdlock, pthread_rwlock_t *, rwlock)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_tryrdlock, pthread_rwlock_t *, rwlock)
MOCK_FUNCTION_END(0)

MOCK_FUNCTION_WITH_CODE(, int, mocked_pthread_rwlock_destroy, pthread_rwlock_t *, rwlock)
MOCK_FUNCTION_END(0)

static SRW_LOCK_HANDLE test_srw_lock_create(bool do_statistics, const char* lock_name)
{
    SRW_LOCK_HANDLE result;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_init(IGNORED_ARG, IGNORED_ARG));
    result = srw_lock_create(do_statistics, lock_name);
    ASSERT_IS_NOT_NULL(result);
    umock_c_reset_all_calls();
    return result;
}

static void test_srw_lock_acquire_exclusive(SRW_LOCK_HANDLE handle)
{
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_wrlock(IGNORED_ARG));
    srw_lock_acquire_exclusive(handle);
    umock_c_reset_all_calls();
}

static void test_srw_lock_acquire_shared(SRW_LOCK_HANDLE handle)
{
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_rdlock(IGNORED_ARG));
    srw_lock_acquire_shared(handle);
    umock_c_reset_all_calls();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types());
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    REGISTER_UMOCK_ALIAS_TYPE(SRW_LOCK_HANDLE, void*);
    REGISTER_UMOCK_ALIAS_TYPE(pthread_rwlock_t*restrict, void*);
    REGISTER_UMOCK_ALIAS_TYPE(const pthread_rwlockattr_t*restrict, void*);
    REGISTER_UMOCK_VALUE_TYPE(va_list, umockvalue_stringify_va_list, umockvalue_are_equal_va_list, umockvalue_copy_va_list, umockvalue_free_va_list);

    REGISTER_STRING_UTILS_GLOBAL_MOCK_HOOK();
    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_GLOBAL_MOCK_FAIL_RETURN(malloc, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(vsprintf_char, NULL);
    REGISTER_GLOBAL_MOCK_FAIL_RETURN(mocked_pthread_rwlock_init, 1);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    real_gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    umock_c_reset_all_calls();
    ASSERT_ARE_EQUAL(int, 0, umock_c_negative_tests_init(), "umock_c_negative_tests_init failed");
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    umock_c_negative_tests_deinit();
}

/* srw_lock_create */

/* Tests_SRS_SRW_LOCK_LINUX_07_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE_DATA. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_002: [ srw_lock_create shall copy the lock_name. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_003: [ srw_lock_create shall initialized the pthread_rwlock_t  by calling pthread_rwlock_init. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_004: [ srw_lock_create shall succeed and return a non-NULL value. ]*/
TEST_FUNCTION(srw_lock_create_succeeds)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_init(IGNORED_ARG, IGNORED_ARG));

    //act
    srw_lock = srw_lock_create(true, "test_lock");

    //assert
    ASSERT_IS_NOT_NULL(srw_lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_destroy(srw_lock);
}

/* Tests_SRS_SRW_LOCK_LINUX_07_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE_DATA. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_005: [ If there are any failures, srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_when_allocating_lock_fails)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG)).SetReturn(NULL);

    //act
    srw_lock = srw_lock_create(true, "test_lock");

    //assert
    ASSERT_IS_NULL(srw_lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_SRW_LOCK_LINUX_07_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE_DATA. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_002: [ srw_lock_create shall copy the lock_name. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_005: [ If there are any failures, srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_when_copy_lock_name_fails)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG)).SetReturn(NULL);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    srw_lock = srw_lock_create(true, "test_lock");

    //assert
    ASSERT_IS_NULL(srw_lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_SRW_LOCK_LINUX_07_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE_DATA. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_002: [ srw_lock_create shall copy the lock_name. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_003: [ srw_lock_create shall initialized the pthread_rwlock_t  by calling pthread_rwlock_init. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_006: [ If initializing the lock failed, srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_when_intializing_lock_fails)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_init(IGNORED_ARG, IGNORED_ARG)).SetReturn(1);
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    srw_lock = srw_lock_create(true, "test_lock");

    //assert
    ASSERT_IS_NULL(srw_lock);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_SRW_LOCK_LINUX_07_001: [ srw_lock_create shall allocate memory for SRW_LOCK_HANDLE_DATA. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_002: [ srw_lock_create shall copy the lock_name. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_003: [ srw_lock_create shall initialized the pthread_rwlock_t  by calling pthread_rwlock_init. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_005: [ If there are any failures, srw_lock_create shall fail and return NULL. ]*/
TEST_FUNCTION(srw_lock_create_fails_when_underlying_calls_fails)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock;
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(vsprintf_char(IGNORED_ARG, IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_init(IGNORED_ARG, IGNORED_ARG));

    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if(umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            srw_lock = srw_lock_create(true, "test_lock");

            // assert
            ASSERT_IS_NULL(srw_lock, "On failed call %zu", i);
        }
    }
}

/* srw_lock_acquire_exclusive */

/* Tests_SRS_SRW_LOCK_LINUX_07_007: [ If handle is NULL, srw_lock_acquire_exclusive shall return. ]*/
TEST_FUNCTION(srw_lock_acquire_exclusive_with_handle_NULL_returns)
{
    //arrange

    //act
    srw_lock_acquire_exclusive(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_SRW_LOCK_LINUX_07_008: [ srw_lock_acquire_exclusive shall lock the pthread_rwlock_t  for writing by calling pthread_rwlock_wrlock. ]*/
TEST_FUNCTION(srw_lock_acquire_exclusive_succeeds)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_wrlock(IGNORED_ARG));

    //act
    srw_lock_acquire_exclusive(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_release_exclusive(srw_lock);
    srw_lock_destroy(srw_lock);
}

/* srw_lock_try_acquire_exclusive */

/* Tests_SRS_SRW_LOCK_LINUX_07_009: [ If handle is NULL, srw_lock_try_acquire_exclusive shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_with_handle_NULL_returns_SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS)
{
    //arrange

    //act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_exclusive(NULL);

    //assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LINUX_07_010: [ Otherwise srw_lock_acquire_exclusive shall apply a write lock on pthread_rwlock_t  only if no other threads are currently holding the pthread_rwlock_t  by calling pthread_rwlock_trywrlock. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_011: [ If pthread_rwlock_trywrlock returns 0, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_succeeds)
{
    //arrange
    SRW_LOCK_TRY_ACQUIRE_RESULT result;
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_trywrlock(IGNORED_ARG));

    //act
    result = srw_lock_try_acquire_exclusive(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_release_exclusive(srw_lock);
    srw_lock_destroy(srw_lock);
}

/* Tests_SRS_SRW_LOCK_LINUX_07_010: [ Otherwise srw_lock_acquire_exclusive shall apply a write lock on pthread_rwlock_t  only if no other threads are currently holding the pthread_rwlock_t  by calling pthread_rwlock_trywrlock. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_012: [ Otherwise, srw_lock_acquire_exclusive shall return SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]*/
TEST_FUNCTION(srw_lock_try_acquire_exclusive_fails_when_apply_write_lock_fails)
{
    //arrange
    SRW_LOCK_TRY_ACQUIRE_RESULT result;
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_trywrlock(IGNORED_ARG)).SetReturn(1);

    //act
    result = srw_lock_try_acquire_exclusive(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_release_exclusive(srw_lock);
    srw_lock_destroy(srw_lock);
}

/* srw_lock_release_exclusive */

/* Tests_SRS_SRW_LOCK_LINUX_07_013: [ If handle is NULL, srw_lock_release_exclusive shall return. ]*/
TEST_FUNCTION(srw_lock_release_exclusive_with_handle_NULL_returns)
{
    //arrange

    //act
    srw_lock_release_exclusive(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
}

/* Tests_SRS_SRW_LOCK_LINUX_07_014: [ srw_lock_release_exclusive shall release the write lock by calling pthread_rwlock_unlock. ]*/
TEST_FUNCTION(srw_lock_release_exclusive_succeeds)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    test_srw_lock_acquire_exclusive(srw_lock);
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_unlock(IGNORED_ARG));

    //act
    srw_lock_release_exclusive(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_destroy(srw_lock);
}

/* srw_lock_acquire_shared */

/* Tests_SRS_SRW_LOCK_LINUX_07_015: [ If handle is NULL, srw_lock_acquire_shared shall return. ]*/
TEST_FUNCTION(srw_lock_acquire_shared_with_handle_NULL_returns)
{
    //arrange

    //act
    srw_lock_acquire_shared(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

}

/* Tests_SRS_SRW_LOCK_LINUX_07_016: [ srw_lock_acquire_shared shall apply a read lock to pthread_rwlock_t  by calling pthread_rwlock_rdlock. ]*/
TEST_FUNCTION(srw_lock_acquire_shared_succeeds)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_rdlock(IGNORED_ARG));

    //act
    srw_lock_acquire_shared(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_release_shared(srw_lock);
    srw_lock_destroy(srw_lock);
}

/* srw_lock_try_acquire_shared */

/* Tests_SRS_SRW_LOCK_LINUX_07_017: [ If handle is NULL then srw_lock_try_acquire_shared shall fail and return SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_with_handle_NULL_returns_SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS)
{
    //arrange

    //act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(NULL);

    //assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_INVALID_ARGS, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LINUX_07_018: [ Otherwise srw_lock_try_acquire_shared shall apply a read lock on pthread_rwlock_t  if there's no writers hold the lock and no writers blocked on the lock by calling pthread_rwlock_tryrdlock. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_019: [ If pthread_rwlock_tryrdlock returns 0, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_OK. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_succeeds)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_tryrdlock(IGNORED_ARG));

    //act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_OK, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_release_shared(srw_lock);
    srw_lock_destroy(srw_lock);
}

/* Tests_SRS_SRW_LOCK_LINUX_07_018: [ Otherwise srw_lock_try_acquire_shared shall apply a read lock on pthread_rwlock_t  if there's no writers hold the lock and no writers blocked on the lock by calling pthread_rwlock_tryrdlock. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_020: [ Otherwise, srw_lock_try_acquire_shared shall return SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE. ]*/
TEST_FUNCTION(srw_lock_try_acquire_shared_fails_when_apply_read_lock_fails)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_tryrdlock(IGNORED_ARG)).SetReturn(1);

    //act
    SRW_LOCK_TRY_ACQUIRE_RESULT result = srw_lock_try_acquire_shared(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(SRW_LOCK_TRY_ACQUIRE_RESULT, SRW_LOCK_TRY_ACQUIRE_COULD_NOT_ACQUIRE, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_release_shared(srw_lock);
    srw_lock_destroy(srw_lock);
}

/* srw_lock_release_shared */

/* Tests_SRS_SRW_LOCK_LINUX_07_021: [ If handle is NULL, srw_lock_release_shared shall return. ]*/
TEST_FUNCTION(srw_lock_release_shared_with_handle_NULL_returns)
{
    //act
    srw_lock_release_shared(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LINUX_07_022: [ srw_lock_release_shared shall release the read lock by calling pthread_rwlock_unlock. ]*/
TEST_FUNCTION(srw_lock_release_shared_succeeds)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");
    test_srw_lock_acquire_shared(srw_lock);

    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_unlock(IGNORED_ARG));

    //act
    srw_lock_release_shared(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    //clean
    srw_lock_destroy(srw_lock);
}

/* srw_lock_destroy */

/* Tests_SRS_SRW_LOCK_LINUX_07_023: [ If handle is NULL then srw_lock_destroy shall return. ]*/
TEST_FUNCTION(srw_lock_destroy_with_handle_NULL_returns)
{

    //act
    srw_lock_destroy(NULL);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/* Tests_SRS_SRW_LOCK_LINUX_07_024: [ srw_lock_destroy shall free the stored lock name. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_025: [ srw_lock_destroy shall destroy the pthread_rwlock_t  by calling pthread_rwlock_destroy. ]*/
/* Tests_SRS_SRW_LOCK_LINUX_07_026: [ srw_lock_destroy shall free the lock handle. ]*/
TEST_FUNCTION(srw_lock_destroy_free_used_resources)
{
    //arrange
    SRW_LOCK_HANDLE srw_lock = test_srw_lock_create(true, "test_lock");

    STRICT_EXPECTED_CALL(free(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_pthread_rwlock_destroy(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    //act
    srw_lock_destroy(srw_lock);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
