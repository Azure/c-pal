// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>


#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes.h"
#include "umock_c/umock_c_negative_tests.h"

#define ENABLE_MOCKS

#include "umock_c/umock_c_prod.h"

    MOCKABLE_FUNCTION(, void, mocked_GetSystemInfo, LPSYSTEM_INFO, lpSystemInfo)


#undef ENABLE_MOCKS

#include "c_pal/sysinfo.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

static const uint32_t TEST_PROC_COUNT = 4;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_UMOCK_ALIAS_TYPE(LPSYSTEM_INFO, void*);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
    TEST_MUTEX_DESTROY(test_serialize_mutex);
}

TEST_FUNCTION_INITIALIZE(init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }

    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

/* sysinfo_get_processor_count */

/* Tests_SRS_SYSINFO_WIN32_01_001: [ sysinfo_get_processor_count shall call GetSystemInfo to obtain the system information. ]*/
/* Tests_SRS_SYSINFO_WIN32_01_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetSystemInfo. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_the_processor_count)
{
    //arrange
    SYSTEM_INFO test_system_info = { 0 };
    test_system_info.dwNumberOfProcessors = TEST_PROC_COUNT;
    STRICT_EXPECTED_CALL(mocked_GetSystemInfo(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpSystemInfo(&test_system_info, sizeof(test_system_info));

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, TEST_PROC_COUNT, proc_count);
}

/* Tests_SRS_SYSINFO_WIN32_01_001: [ sysinfo_get_processor_count shall call GetSystemInfo to obtain the system information. ]*/
/* Tests_SRS_SYSINFO_WIN32_01_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetSystemInfo. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_the_processor_count_33)
{
    //arrange
    SYSTEM_INFO test_system_info = { 0 };
    test_system_info.dwNumberOfProcessors = 33;
    STRICT_EXPECTED_CALL(mocked_GetSystemInfo(IGNORED_ARG))
        .CopyOutArgumentBuffer_lpSystemInfo(&test_system_info, sizeof(test_system_info));

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, 33, proc_count);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
