// Copyright(C) Microsoft Corporation.All rights reserved.


#include "sysinfo_linux_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
    MOCKABLE_FUNCTION(, long, mocked_sysconf, int, name)
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
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

/* sysinfo_get_processor_count */

/* Tests_SRS_SYSINFO_LINUX_01_001: [ sysinfo_get_processor_count shall call sysconf with SC_NPROCESSORS_ONLN to obtain the number of configured processors. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_the_result_of_sysconf)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_sysconf(_SC_NPROCESSORS_ONLN))
        .SetReturn(TEST_PROC_COUNT);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, TEST_PROC_COUNT, proc_count);
}

/* Tests_SRS_SYSINFO_LINUX_01_001: [ sysinfo_get_processor_count shall call sysconf with SC_NPROCESSORS_ONLN to obtain the number of configured processors. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_the_result_of_sysconf_33)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_sysconf(_SC_NPROCESSORS_ONLN))
        .SetReturn(33);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, 33, proc_count);
}

/* Tests_SRS_SYSINFO_LINUX_01_002: [ If any error occurs, sysinfo_get_processor_count shall return 0. ]*/
TEST_FUNCTION(when_sysconf_fails_sysinfo_get_processor_count_returns_0)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_sysconf(_SC_NPROCESSORS_ONLN))
        .SetReturn(-1);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, 0, proc_count);
}

/* Tests_SRS_SYSINFO_LINUX_01_003: [ If sysconf returns a number bigger than UINT32_MAX, sysinfo_get_processor_count shall fail and return 0. ]*/
TEST_FUNCTION(when_sysconf_returns_UINT32_MAX_sysinfo_get_processor_count_succeeds)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_sysconf(_SC_NPROCESSORS_ONLN))
        .SetReturn(UINT32_MAX);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, UINT32_MAX, proc_count);
}

/* Tests_SRS_SYSINFO_LINUX_01_003: [ If sysconf returns a number bigger than UINT32_MAX, sysinfo_get_processor_count shall fail and return 0. ]*/
TEST_FUNCTION(when_sysconf_returns_more_than_UINT32_MAX_sysinfo_get_processor_count_returns_0)
{
    //arrange
    STRICT_EXPECTED_CALL(mocked_sysconf(_SC_NPROCESSORS_ONLN))
        .SetReturn(UINT32_MAX + 1);

    //act
    uint32_t proc_count = sysinfo_get_processor_count();

    //assert
    ASSERT_ARE_EQUAL(uint32_t, 0, proc_count);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
