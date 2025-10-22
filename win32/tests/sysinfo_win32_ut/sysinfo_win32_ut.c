// Copyright(C) Microsoft Corporation.All rights reserved.



#include "sysinfo_win32_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
    MOCKABLE_FUNCTION(, DWORD, mocked_GetActiveProcessorCount, WORD, GroupNumber)
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

static const uint32_t TEST_PROC_COUNT = 4;

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");

    REGISTER_UMOCK_ALIAS_TYPE(WORD, uint16_t);
    REGISTER_UMOCK_ALIAS_TYPE(DWORD, uint32_t);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
    umock_c_negative_tests_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

/* sysinfo_get_processor_count */

/* Tests_SRS_SYSINFO_WIN32_43_001: [ sysinfo_get_processor_count shall call GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) to obtain the number of processors. ]*/
/* Tests_SRS_SYSINFO_WIN32_43_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetActiveProcessorCount. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_the_processor_count)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_GetActiveProcessorCount(ALL_PROCESSOR_GROUPS))
        .SetReturn(TEST_PROC_COUNT);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, TEST_PROC_COUNT, proc_count);
}

/* Tests_SRS_SYSINFO_WIN32_43_001: [ sysinfo_get_processor_count shall call GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) to obtain the number of processors. ]*/
/* Tests_SRS_SYSINFO_WIN32_43_002: [ sysinfo_get_processor_count shall return the processor count as returned by GetActiveProcessorCount. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_the_processor_count_33)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_GetActiveProcessorCount(ALL_PROCESSOR_GROUPS))
        .SetReturn(33);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, 33, proc_count);
}

/* Tests_SRS_SYSINFO_WIN32_43_001: [ sysinfo_get_processor_count shall call GetActiveProcessorCount(ALL_PROCESSOR_GROUPS) to obtain the number of processors. ]*/
/* Tests_SRS_SYSINFO_WIN32_43_003: [ If there are any failures, sysinfo_get_processor_count shall fail and return zero. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_fails)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_GetActiveProcessorCount(ALL_PROCESSOR_GROUPS))
        .SetReturn(0);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, 0, proc_count);
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)