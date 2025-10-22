// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "sync_linux_ut_pch.h"
#undef ENABLE_MOCKS_DECL

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#include "mock_sync.h"
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS


static bool check_timeout;
static uint32_t expected_timeout_ms;
static int expected_return_val;

static int hook_mock_syscall(long call_code, int* uaddr, int futex_op, int val, const struct timespec* timeout, int* uaddr2, int val3)
{
    /* Tests_SRS_SYNC_LINUX_43_001: [ wait_on_address shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ] */
    if(check_timeout)
    {
        ASSERT_ARE_EQUAL(long, (expected_timeout_ms / 1000), (long)timeout->tv_sec);
        ASSERT_ARE_EQUAL(long, (expected_timeout_ms % 1000)*1e6, timeout->tv_nsec);
    }
    return expected_return_val;
}

TEST_DEFINE_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    REGISTER_GLOBAL_MOCK_HOOK(mock_syscall, hook_mock_syscall)
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    expected_return_val = 0;
    mock_errno = 0;
    check_timeout = false;
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

/* Tests_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
/* Tests_SRS_SYNC_LINUX_43_003: [ If the value at address changes to a value different from compare_value then wait_on_address shall return WAIT_ON_ADDRESS_OK. ] */

TEST_FUNCTION(wait_on_address_calls_syscall_successfully)
{
    //arrange
    volatile_atomic int32_t var;
    (void)atomic_exchange(&var, INT32_MAX);
    check_timeout = true;
    expected_timeout_ms = 100;
    expected_return_val = 0;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, INT32_MAX, IGNORED_ARG, NULL, 0));

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, INT32_MAX, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address should have returned ok");
}

/* Tests_SRS_SYNC_LINUX_05_001: [ wait_on_address_64 shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ] */
/* Tests_SRS_SYNC_LINUX_05_002: [ wait_on_address_64 shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
/* Tests_SRS_SYNC_LINUX_05_003: [ If the value at address changes to a value different from compare_value then wait_on_address_64 shall return WAIT_ON_ADDRESS_OK. ] */
TEST_FUNCTION(wait_on_address_calls_64_syscall_successfully)
{
    //arrange
    volatile_atomic int64_t var;
    (void)atomic_exchange(&var, INT64_MAX);
    check_timeout = true;
    expected_timeout_ms = 100;
    expected_return_val = 0;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, var, IGNORED_ARG, NULL, 0));

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, INT64_MAX, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address_64 should have returned ok");
}

/* Tests_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
/* Tests_SRS_SYNC_LINUX_43_004: [ Otherwise, wait_on_address shall return WAIT_ON_ADDRESS_ERROR.] */
TEST_FUNCTION(wait_on_address_calls_sycall_unsuccessfully)
{
    //arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    check_timeout = true;
    expected_timeout_ms = 1500;
    expected_return_val = -1;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0));

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, val, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_ERROR, return_val, "wait_on_address should have returned error");
}

/* Tests_SRS_SYNC_LINUX_05_001: [ wait_on_address_64 shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ] */
/* Tests_SRS_SYNC_LINUX_05_002: [ wait_on_address_64 shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
/* Tests_SRS_SYNC_LINUX_05_006: [ Otherwise, wait_on_address_64 shall return WAIT_ON_ADDRESS_ERROR. ] */
TEST_FUNCTION(wait_on_address_64_calls_syscall_unsuccessfully)
{
    //arrange
    volatile_atomic int64_t var;
    int64_t val = INT64_MAX;
    (void)atomic_exchange(&var, val);
    check_timeout = true;
    expected_timeout_ms = 1500;
    expected_return_val = -1;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0));

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, val, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_ERROR, return_val, "wait_on_address_64 should have returned error");
}

/* Tests_SRS_SYNC_LINUX_01_001: [ if syscall returns a non-zero value and errno is EAGAIN, wait_on_address shall return WAIT_ON_ADDRESS_OK. ] */
TEST_FUNCTION(when_syscall_fails_and_errno_is_EAGAIN_wait_on_address_succeeds)
{
    //arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    mock_errno = EAGAIN;

    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0))
        .SetReturn(-1);

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, val, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address should have returned ok");
}

/* Tests_SRS_SYNC_LINUX_05_001: [ wait_on_address_64 shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ] */
/* Tests_SRS_SYNC_LINUX_05_002: [ wait_on_address_64 shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
/* Tests_SRS_SYNC_LINUX_05_004: [ If syscall returns a non-zero value and errno is EAGAIN, wait_on_address_64 shall return WAIT_ON_ADDRESS_OK. ] */
TEST_FUNCTION(when_syscall_fails_and_errno_is_EAGAIN_wait_on_address_64_succeeds)
{
    //arrange
    volatile_atomic int64_t var;
    int64_t val = INT64_MAX;
    (void)atomic_exchange(&var, val);
    mock_errno = EAGAIN;

    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0))
        .SetReturn(-1);

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, val, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address_64 should have returned ok");
}

/* Tests_SRS_SYNC_LINUX_24_001: [ if syscall returns a non-zero value and errno is ETIMEDOUT, wait_on_address shall return WAIT_ON_ADDRESS_TIMEOUT. ] */
TEST_FUNCTION(when_syscall_fails_and_errno_is_ETIME_wait_on_address_fails)
{
    //arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    mock_errno = ETIMEDOUT;

    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0))
        .SetReturn(-1);

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, val, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address should have returned timeout");
}

/* Tests_SRS_SYNC_LINUX_05_001: [ wait_on_address_64 shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ] */
/* Tests_SRS_SYNC_LINUX_05_002: [ wait_on_address_64 shall call syscall to wait on value at address to change to a value different than the one provided in compare_value. ] */
/* Tests_SRS_SYNC_LINUX_05_005: [ If syscall returns a non-zero value and errno is ETIMEDOUT, wait_on_address_64 shall return WAIT_ON_ADDRESS_TIMEOUT. ]*/
TEST_FUNCTION(when_syscall_fails_and_errno_is_ETIME_wait_on_address_64_fails)
{
    //arrange
    volatile_atomic int64_t var;
    int64_t val = INT64_MAX;
    (void)atomic_exchange(&var, val);
    mock_errno = ETIMEDOUT;

    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0))
        .SetReturn(-1);

    //act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, val, expected_timeout_ms);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address_64 should have returned timeout");
}

/* Tests_SRS_SYNC_LINUX_43_005: [ wake_by_address_all shall call syscall to wake all listeners listening on address. ] */
TEST_FUNCTION(wake_by_address_all_calls_sycall)
{
    //arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0));

    //act
    wake_by_address_all(&var);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/* Tests_SRS_SYNC_LINUX_05_007: [ wake_by_address_all_64 shall call syscall to wake all listeners listening on address. ] */
TEST_FUNCTION(wake_by_address_all_64_calls_sycall)
{
    //arrange
    volatile_atomic int64_t var;
    int64_t val = INT64_MAX;
    (void)atomic_exchange(&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0));

    //act
    wake_by_address_all_64(&var);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/* Tests_SRS_SYNC_LINUX_43_006: [ wake_by_address_single shall call syscall to wake any single listener listening on address. ] */
TEST_FUNCTION(wake_by_address_single_calls_sycall)
{
    //arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0));

    //act
    wake_by_address_single(&var);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/* Tests_SRS_SYNC_LINUX_05_008: [ wake_by_address_single_64 shall call syscall to wake any single listener listening on address. ] */
TEST_FUNCTION(wake_by_address_single_64_calls_sycall)
{
    //arrange
    volatile_atomic int64_t var;
    int64_t val = INT64_MAX;
    (void)atomic_exchange(&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0));

    //act
    wake_by_address_single_64(&var);

    //assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)