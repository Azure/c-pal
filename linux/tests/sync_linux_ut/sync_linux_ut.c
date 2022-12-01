// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <limits.h>
#include <time.h>
#include <errno.h>


#include <syscall.h>
#include <linux/futex.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes.h"

#include "c_pal/interlocked.h"
#include "c_pal/sync.h"

// No idea why iwyu warns about this since we include time.h but...
// IWYU pragma: no_forward_declare timespec

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define ENABLE_MOCKS
#include "mock_sync.h"
#undef ENABLE_MOCKS


static bool check_timeout;
static uint32_t expected_timeout_ms;
static int expected_return_val;
int mock_errno;
static int hook_mock_syscall(long call_code, int* uaddr, int futex_op, int val, const struct timespec* timeout, int* uaddr2, int val3)
{
    /*Tests_SRS_SYNC_LINUX_43_001: [ wait_on_address shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ]*/
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
    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    REGISTER_GLOBAL_MOCK_HOOK(mock_syscall, hook_mock_syscall)
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    TEST_MUTEX_DESTROY(g_testByTest);
}

TEST_FUNCTION_INITIALIZE(f)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
    expected_return_val = 0;
    mock_errno = 0;
    check_timeout = false;
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAIT_PRIVATE, compare_value, timeout_struct, NULL, NULL. ]*/
/*Tests_SRS_SYNC_LINUX_43_003: [ wait_on_address shall return true if syscall returns 0.]*/

TEST_FUNCTION(wait_on_address_calls_syscall_successfully)
{
    ///arrange
    volatile_atomic int32_t var;
    (void)atomic_exchange(&var, INT32_MAX);
    check_timeout = true;
    expected_timeout_ms = 100;
    expected_return_val = 0;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int32_t*)&var, FUTEX_WAIT_PRIVATE, INT32_MAX, IGNORED_ARG, NULL, 0));

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, INT32_MAX, expected_timeout_ms);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address should have returned ok");
}

/*Tests_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAIT_PRIVATE, *compare_address, timeout_struct, NULL, NULL. ]*/
/*Tests_SRS_SYNC_LINUX_43_004: [ Otherwise, wait_on_address shall return false.*/
TEST_FUNCTION(wait_on_address_calls_sycall_unsuccessfully)
{
    ///arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    check_timeout = true;
    expected_timeout_ms = 1500;
    expected_return_val = -1;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0));

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, val, expected_timeout_ms);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_ERROR, return_val, "wait_on_address should have returned error");
}

/* Tests_SRS_SYNC_LINUX_01_001: [ if syscall returns a non-zero value and errno is EAGAIN, wait_on_address shall return true. ]*/
TEST_FUNCTION(when_syscall_fails_and_errno_is_EAGAIN_wait_on_address_succeeds)
{
    ///arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    mock_errno = EAGAIN;

    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0))
        .SetReturn(-1);

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, val, expected_timeout_ms);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address should have returned ok");
}

/* Tests_SRS_SYNC_LINUX_24_001: [ if syscall returns a non-zero value and errno is ETIME, wait_on_address shall return WAIT_ON_ADDRESS_TIMEOUT. ]*/
TEST_FUNCTION(when_syscall_fails_and_errno_is_ETIME_wait_on_address_fails)
{
    ///arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    mock_errno = ETIME;

    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, 0))
        .SetReturn(-1);

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, val, expected_timeout_ms);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address should have returned timeout");
}

/*Tests_SRS_SYNC_LINUX_43_005: [ wake_by_address_all shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, NULL. ]*/
TEST_FUNCTION(wake_by_address_all_calls_sycall)
{
    ///arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, 0));

    ///act
    wake_by_address_all(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/*Tests_SRS_SYNC_LINUX_43_006: [ wake_by_address_single shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, NULL. ]*/
TEST_FUNCTION(wake_by_address_single_calls_sycall)
{
    ///arrange
    volatile_atomic int32_t var;
    int32_t val = INT32_MAX;
    (void)atomic_exchange(&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, 0));

    ///act
    wake_by_address_single(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
