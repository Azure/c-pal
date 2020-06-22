// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdlib>
#include <cstddef>
#else
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#endif

#include <sys/syscall.h>
#include <linux/futex.h>
#include <time.h>
#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"

#include "sync.h"

static TEST_MUTEX_HANDLE g_testByTest;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#ifdef __cplusplus
extern "C"
{
#endif

#define ENABLE_MOCKS
#include "mock_sync.h"
#undef ENABLE_MOCKS

#ifdef __cplusplus
}
#endif

static uint32_t expected_timeout_ms;
static int expected_return_val;
static bool hook_mock_syscall(long call_code, int* uaddr, int futex_op, int val, const struct timespec* timeout, int* uaddr2, int val3)
{
    /*Tests_SRS_SYNC_LINUX_43_001: [ wait_on_address shall initialize a timespec struct with .tv_nsec equal to timeout_ms* 10^6. ]*/
    ASSERT_ARE_EQUAL(long, expected_timeout_ms*10e6, timeout->tv_nsec);
    return expected_return_val;
}


BEGIN_TEST_SUITE(sync_win32_unittests)

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

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/*Tests_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAIT_PRIVATE, *compare_address, timeout_struct, NULL, NULL. ]*/
/*Tests_SRS_SYNC_LINUX_43_003: [ wait_on_address shall return true if syscall returns 0.]*/

TEST_FUNCTION(wait_on_address_calls_syscall_successfully)
{
    ///arrange
    volatile int32_t var;
    int32_t val = INT32_MAX;
    InterlockedExchange((volatile LONG*)&var, val);
    expected_timeout_ms = 1000;
    expected_return_val = 0;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int32_t*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, NULL));

    ///act
    bool return_val = wait_on_address(&var, val, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_IS_TRUE(return_val, "Return value is incorrect.");
}

/*Tests_SRS_SYNC_LINUX_43_002: [ wait_on_address shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAIT_PRIVATE, *compare_address, timeout_struct, NULL, NULL. ]*/
/*Tests_SRS_SYNC_LINUX_43_004: [ wait_on_address shall return false if syscall does not return 0.]*/
TEST_FUNCTION(wait_on_address_calls_sycall_unsuccessfully)
{
    ///arrange
    volatile int32_t var;
    int32_t val = INT32_MAX;
    InterlockedExchange((volatile LONG*)&var, val);
    expected_timeout_ms = 1000;
    expected_return_val = -1;
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAIT_PRIVATE, val, IGNORED_ARG, NULL, NULL));

    ///act
    bool return_val = wait_on_address(&var, val, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_IS_FALSE(return_val, "Return value is incorrect.");
}

/*Tests_SRS_SYNC_LINUX_43_005: [ wake_by_address_all shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, NULL. ]*/
TEST_FUNCTION(wake_by_address_all_calls_sycall)
{
    ///arrange
    volatile int32_t var;
    int32_t val = INT32_MAX;
    InterlockedExchange((volatile LONG*)&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, INT_MAX, NULL, NULL, NULL));
    ///act
    wake_by_address_all(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/*Tests_SRS_SYNC_LINUX_43_006: [ wake_by_address_single shall call syscall from sys/syscall.h with arguments SYS_futex, address, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, NULL. ]*/
TEST_FUNCTION(wake_by_address_single_calls_sycall)
{
    ///arrange
    volatile int32_t var;
    int32_t val = INT32_MAX;
    InterlockedExchange((volatile LONG*)&var, val);
    STRICT_EXPECTED_CALL(mock_syscall(SYS_futex, (int*)&var, FUTEX_WAKE_PRIVATE, 1, NULL, NULL, NULL));
    ///act
    wake_by_address_all(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}
END_TEST_SUITE(sync_win32_unittests)
