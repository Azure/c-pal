// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

#include "windows.h"
#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_windows.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define ENABLE_MOCKS
#include "mock_sync.h"
#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"

#include "c_pal/sync.h"

TEST_DEFINE_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);
IMPLEMENT_UMOCK_C_ENUM_TYPE(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_RESULT_VALUES);

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error));
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types());
    REGISTER_UMOCK_ALIAS_TYPE(SIZE_T, size_t);
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
    umock_c_deinit();

    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(f)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleans)
{
}

/*Tests_SRS_SYNC_WIN32_43_001: [ wait_on_address shall call WaitOnAddress from windows.h with address as Address, a pointer to the value compare_value as CompareAddress, 4 as AddressSize and timeout_ms as dwMilliseconds. ]*/
/*Tests_SRS_SYNC_WIN32_24_003: [ If WaitOnAddress succeeds, wait_on_address shall return WAIT_ON_ADDRESS_OK. ]*/
TEST_FUNCTION(wait_on_address_calls_WaitOnAddress_successfully)
{
    ///arrange
    volatile int32_t var = 0;
    int32_t expected_val = INT32_MAX;
    (void)InterlockedExchange((volatile LONG*)&var, INT32_MAX);
    uint32_t timeout = 1000;
    STRICT_EXPECTED_CALL(mock_WaitOnAddress((volatile VOID*)&var, IGNORED_ARG, (SIZE_T)4, (DWORD)timeout))
        .ValidateArgumentBuffer(2, &expected_val, sizeof(expected_val))
        .SetReturn(true);

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, INT32_MAX, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address should have returned ok");
}

/* Tests_SRS_SYNC_WIN32_05_001: [ wait_on_address_64 shall call WaitOnAddress from the Windows API. ] */
/* Tests_SRS_SYNC_WIN32_05_004: [ If WaitOnAddress detects a change in value at address, wait_on_address_64 shall succeed and return WAIT_ON_ADDRESS_OK. ] */
TEST_FUNCTION(wait_on_address_64_calls_WaitOnAddress_successfully)
{
    ///arrange
    volatile int64_t var;
    int64_t expected_val = INT64_MAX;
    (void)InterlockedExchange64((volatile LONG64*)&var, INT64_MAX);
    uint32_t timeout = 1000;
    STRICT_EXPECTED_CALL(mock_WaitOnAddress((volatile VOID*)&var, IGNORED_ARG, (SIZE_T)8, (DWORD)timeout))
        .ValidateArgumentBuffer(2, &expected_val, sizeof(expected_val))
        .SetReturn(true);

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, INT64_MAX, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_OK, return_val, "wait_on_address_64 should have returned ok");
}

/*Tests_SRS_SYNC_WIN32_43_001: [ wait_on_address shall call WaitOnAddress from windows.h with address as Address, a pointer to the value compare_value as CompareAddress, 4 as AddressSize and timeout_ms as dwMilliseconds. ]*/
/*Tests_SRS_SYNC_WIN32_24_002: [ If WaitOnAddress fails due to any other reason, wait_on_address shall fail and return WAIT_ON_ADDRESS_ERROR. ]*/
TEST_FUNCTION(wait_on_address_calls_fails_when_WaitOnAddress_fails_due_to_timeout)
{
    ///arrange
    volatile int32_t var;
    int32_t expected_val = INT32_MAX;
    (void)InterlockedExchange((volatile LONG*)&var, INT32_MAX);
    uint32_t timeout = 1000;
    STRICT_EXPECTED_CALL(mock_WaitOnAddress((volatile VOID*)&var, IGNORED_ARG, (SIZE_T)4, (DWORD)timeout))
        .ValidateArgumentBuffer(2, &expected_val, sizeof(expected_val))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mock_GetLastError());

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, INT32_MAX, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_ERROR, return_val, "wait_on_address should have returned error");
}

/* Tests_SRS_SYNC_WIN32_05_001: [ wait_on_address_64 shall call WaitOnAddress from the Windows API ] */
/* Tests_SRS_SYNC_WIN32_05_002: [ If the value at address does not change until the timeout_ms timeout is hit, wait_on_address_64 shall fail and return WAIT_ON_ADDRESS_TIMEOUT. ] */
TEST_FUNCTION(wait_on_address_64_calls_fails_when_WaitOnAddress_fails_due_to_timeout)
{
    ///arrange
    volatile int64_t var;
    int64_t expected_val = INT64_MAX;
    (void)InterlockedExchange64((volatile LONG64*)&var, INT64_MAX);
    uint32_t timeout = 1000;
    STRICT_EXPECTED_CALL(mock_WaitOnAddress((volatile VOID*)&var, IGNORED_ARG, (SIZE_T)8, (DWORD)timeout))
        .ValidateArgumentBuffer(2, &expected_val, sizeof(expected_val))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mock_GetLastError());

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, INT64_MAX, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_ERROR, return_val, "wait_on_address_64 should have returned error");
}

/*Tests_SRS_SYNC_WIN32_43_001: [ wait_on_address shall call WaitOnAddress from windows.h with address as Address, a pointer to the value compare_value as CompareAddress, 4 as AddressSize and timeout_ms as dwMilliseconds. ]*/
/*Tests_SRS_SYNC_WIN32_24_001: [ If WaitOnAddress fails due to timeout, wait_on_address shall fail and return WAIT_ON_ADDRESS_TIMEOUT. ]*/
TEST_FUNCTION(wait_on_address_calls_WaitOnAddress_unsuccessfully)
{
    ///arrange
    volatile int32_t var;
    int32_t expected_val = INT32_MAX;
    (void)InterlockedExchange((volatile LONG*)&var, INT32_MAX);
    uint32_t timeout = 1000;
    STRICT_EXPECTED_CALL(mock_WaitOnAddress((volatile VOID*)&var, IGNORED_ARG, (SIZE_T)4, (DWORD)timeout))
        .ValidateArgumentBuffer(2, &expected_val, sizeof(expected_val))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mock_GetLastError()).SetReturn(ERROR_TIMEOUT);

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address(&var, INT32_MAX, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address should have returned timeout");
}

/* Tests_SRS_SYNC_WIN32_05_001: [ wait_on_address_64 shall call WaitOnAddress from the Windows API ] */
/* Tests_SRS_SYNC_WIN32_05_003: [ If WaitOnAddress fails due to any other reason, wait_on_address_64 shall fail and return WAIT_ON_ADDRESS_ERROR. ] */
TEST_FUNCTION(wait_on_address_64_calls_WaitOnAddress_unsuccessfully)
{
    ///arrange
    volatile int64_t var = 0;
    int64_t expected_val = INT64_MAX;
    (void)InterlockedExchange64((volatile LONG64*)&var, INT64_MAX);
    uint32_t timeout = 1000;
    STRICT_EXPECTED_CALL(mock_WaitOnAddress((volatile VOID*)&var, IGNORED_ARG, (SIZE_T)8, (DWORD)timeout))
        .ValidateArgumentBuffer(2, &expected_val, sizeof(expected_val))
        .SetReturn(false);
    STRICT_EXPECTED_CALL(mock_GetLastError()).SetReturn(ERROR_TIMEOUT);

    ///act
    WAIT_ON_ADDRESS_RESULT return_val = wait_on_address_64(&var, INT64_MAX, timeout);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
    ASSERT_ARE_EQUAL(WAIT_ON_ADDRESS_RESULT, WAIT_ON_ADDRESS_TIMEOUT, return_val, "wait_on_address_64 should have returned timeout");
}

/*Tests_SRS_SYNC_WIN32_43_003: [ wake_by_address_all shall call WakeByAddressAll from windows.h with address as Address. ]*/
TEST_FUNCTION(wake_by_address_all_calls_WakeByAddressAll)
{
    ///arrange
    volatile int32_t var;
    int32_t val = INT32_MAX;
    (void)InterlockedExchange((volatile LONG*)&var, val);
    STRICT_EXPECTED_CALL(mock_WakeByAddressAll((PVOID)&var));
    ///act
    wake_by_address_all(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/* Tests_SRS_SYNC_WIN32_05_005: [ wake_by_address_all_64 shall call WakeByAddressAll from the Windows API to notify all threads waiting on address. ] */
TEST_FUNCTION(wake_by_address_all_64_calls_WakeByAddressAll)
{
    ///arrange
    volatile int64_t var;
    int64_t val = INT64_MAX;
    (void)InterlockedExchange64((volatile LONG64*)&var, val);
    STRICT_EXPECTED_CALL(mock_WakeByAddressAll((PVOID)&var));
    ///act
    wake_by_address_all_64(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/*Tests_SRS_SYNC_WIN32_43_004: [ wake_by_address_single shall call WakeByAddressSingle from windows.h with address as Address. ]*/
TEST_FUNCTION(wake_by_address_single_calls_WakeByAddressSingle)
{
    ///arrange
    volatile int32_t var;
    int32_t val = INT32_MAX;
    (void)InterlockedExchange((volatile LONG*)&var, val);
    STRICT_EXPECTED_CALL(mock_WakeByAddressSingle((PVOID)&var));
    ///act
    wake_by_address_single(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}

/* Tests_SRS_SYNC_WIN32_05_006: [ wake_by_address_single_64 shall call WakeByAddressSingle from the Windows API to notify a single thread waiting on address. ] */
TEST_FUNCTION(wake_by_address_single_64_calls_WakeByAddressSingle)
{
    ///arrange
    volatile int64_t var;
    int64_t val = INT32_MAX;
    (void)InterlockedExchange64((volatile LONG64*)&var, val);
    STRICT_EXPECTED_CALL(mock_WakeByAddressSingle((PVOID)&var));
    ///act
    wake_by_address_single_64(&var);

    ///assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls(), "Actual calls differ from expected calls");
}
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
