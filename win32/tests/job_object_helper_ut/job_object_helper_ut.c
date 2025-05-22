// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>

#include "windows.h"
#include "jobapi2.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_stdint.h"
#include "umock_c/umocktypes_windows.h"

#include "c_pal/interlocked.h"
#include "c_pal/interlocked_hl.h"

#define ENABLE_MOCKS
    #include "c_pal/gballoc_hl.h"
    #include "c_pal/gballoc_hl_redirect.h"

    MOCKABLE_FUNCTION(, BOOL, mocked_AssignProcessToJobObject, HANDLE, hJob, HANDLE, hProcess);

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "c_pal/job_object_helper.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define test_total_physical_memory 20480

MOCK_FUNCTION_WITH_CODE(, HANDLE, mocked_CreateJobObject, LPSECURITY_ATTRIBUTES, lpJobAttributes, LPCSTR, lpName)
MOCK_FUNCTION_END(real_gballoc_hl_malloc(1))

MOCK_FUNCTION_WITH_CODE(, HANDLE, mocked_CreateJobObjectA, LPSECURITY_ATTRIBUTES, lpJobAttributes, LPCSTR, lpName)
MOCK_FUNCTION_END(real_gballoc_hl_malloc(1))

MOCK_FUNCTION_WITH_CODE(, HANDLE, mocked_GetCurrentProcess)
MOCK_FUNCTION_END(real_gballoc_hl_malloc(1))

MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_CloseHandle, HANDLE, hObject)
{
    real_gballoc_hl_free(hObject);
}
MOCK_FUNCTION_END(TRUE)


MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_GlobalMemoryStatusEx, LPMEMORYSTATUSEX, lpBuffer)
{
    ASSERT_IS_NOT_NULL(lpBuffer);
    ASSERT_ARE_EQUAL(size_t, lpBuffer->dwLength, sizeof(MEMORYSTATUSEX));
    (void)memset(lpBuffer, 0, sizeof(MEMORYSTATUSEX));
    lpBuffer->dwLength = sizeof(MEMORYSTATUSEX);
    lpBuffer->ullTotalPhys = test_total_physical_memory;
}
MOCK_FUNCTION_END(TRUE)

static JOBOBJECT_EXTENDED_LIMIT_INFORMATION captured_extended_limit_information;
static JOBOBJECT_CPU_RATE_CONTROL_INFORMATION captured_cpu_rate_control_information;
MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_SetInformationJobObject, HANDLE, hJob, JOBOBJECTINFOCLASS, JobObjectInformationClass, LPVOID, lpJobObjectInformation, DWORD, cbJobObjectInformationLength)
{
    ASSERT_IS_NOT_NULL(lpJobObjectInformation);
    if (JobObjectInformationClass == JobObjectExtendedLimitInformation)
    {
        (void)memcpy(&captured_extended_limit_information, lpJobObjectInformation, cbJobObjectInformationLength);
    }
    else if (JobObjectInformationClass == JobObjectCpuRateControlInformation)
    {
        (void)memcpy(&captured_cpu_rate_control_information, lpJobObjectInformation, cbJobObjectInformationLength);
    }
    else
    {
        ASSERT_FAIL("Unexpected JobObjectInformationClass passed to SetInformationJobObject");
    }
}
MOCK_FUNCTION_END(TRUE)

static void setup_job_object_helper_create_expectations()
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CreateJobObject(NULL, NULL))
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .SetReturn(TRUE)
        .CallCannotFail();
}

static void setup_job_object_helper_limit_memory_expectations(void)
{
    STRICT_EXPECTED_CALL(mocked_GlobalMemoryStatusEx(IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_SetInformationJobObject(IGNORED_ARG, JobObjectExtendedLimitInformation, IGNORED_ARG, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
}

static void setup_job_object_helper_limit_cpu_expectations(void)
{
    STRICT_EXPECTED_CALL(mocked_SetInformationJobObject(IGNORED_ARG, JobObjectCpuRateControlInformation, IGNORED_ARG, sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION)))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
}

static void setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations(void)
{
    STRICT_EXPECTED_CALL(malloc(IGNORED_ARG));

    STRICT_EXPECTED_CALL(mocked_CreateJobObjectA(IGNORED_ARG, "job_name"))
        .SetFailReturn(NULL);
}

static void setup_job_object_helper_set_job_limits_to_current_process_process_assign_expectations(void)
{
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
}

static void setup_job_object_helper_set_job_limits_to_current_process_expectations(void)
{
    setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    setup_job_object_helper_set_job_limits_to_current_process_process_assign_expectations();
}


BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL), "real_gballoc_hl_init failed");
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_stdint_register_types(), "umocktypes_stdint_register_types failed");
    ASSERT_ARE_EQUAL(int, 0, umocktypes_windows_register_types(), "umocktypes_windows_register_types failed");

    REGISTER_GBALLOC_HL_GLOBAL_MOCK_HOOK();

    REGISTER_UMOCK_ALIAS_TYPE(LPMEMORYSTATUSEX, void*);
    REGISTER_UMOCK_ALIAS_TYPE(JOBOBJECTINFOCLASS, int);

    umock_c_negative_tests_init();

}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    umock_c_negative_tests_deinit();
    umock_c_deinit();
    real_gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(init)
{
    umock_c_reset_all_calls();
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_016: [ job_object_helper_create shall allocate a JOB_OBJECT_HELPER object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_024: [ job_object_helper_create shall call CreateJobObject to create a new job object passing NULL for both lpJobAttributes and lpName. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_025: [ job_object_helper_create shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_026: [ job_object_helper_create shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_027: [ job_object_helper_create shall call CloseHandle to close the handle of the current process. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_019: [ job_object_helper_create shall succeed and return the JOB_OBJECT_HELPER object. ]*/
TEST_FUNCTION(job_object_helper_create_succeeds)
{
    // arrange
    setup_job_object_helper_create_expectations();

    // act
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);

}

/*Tests_SRS_JOB_OBJECT_HELPER_18_018: [ If there are any failures, job_object_helper_create shall fail and return NULL. ]*/
TEST_FUNCTION(job_object_helper_create_fails)
{
    // arrange
    setup_job_object_helper_create_expectations();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_create();

            // assert
            ASSERT_IS_NULL(result);
        }
    }

}

/*Tests_SRS_JOB_OBJECT_HELPER_18_033: [ job_object_helper_dispose shall call CloseHandle to close the handle to the job object. ]*/
TEST_FUNCTION(job_object_helper_dispose_succeeds)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_035: [ If job_object_helper is NULL, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_memory_with_NULL_job_object_helper)
{
    // arrange

    // act
    int result = job_object_helper_limit_memory(NULL, 42);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

}

/*Tests_SRS_JOB_OBJECT_HELPER_18_036: [ If percent_physical_memory is 0, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_037: [ If percent_physical_memory is greater than 100, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_memory_with_invalid_percent_physical_memory)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();

    uint32_t invalid_values[] = {0, 101, 143};

    for (int i=0; i < sizeof(invalid_values)/sizeof(invalid_values[0]); i++)
    {
        // act
        int result = job_object_helper_limit_memory(job_object_helper, invalid_values[i]);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        // nothing is expected, nothing should be done.
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_023: [ job_object_helper_limit_memory shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_039: [ job_object_helper_limit_memory shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_042: [ job_object_helper_limit_memory shall succeed and return 0. ]*/
TEST_FUNCTION(job_object_helper_limit_memory_succeeds)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();
    setup_job_object_helper_limit_memory_expectations();

    // act
    int result = job_object_helper_limit_memory(job_object_helper, 42);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    ASSERT_ARE_EQUAL(unsigned_long, JOB_OBJECT_LIMIT_JOB_MEMORY, captured_extended_limit_information.BasicLimitInformation.LimitFlags);
    ASSERT_ARE_EQUAL(size_t, test_total_physical_memory * 42 /100, captured_extended_limit_information.JobMemoryLimit);

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_041: [ If there are any failures, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_memory_fails)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();
    setup_job_object_helper_limit_memory_expectations();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = job_object_helper_limit_memory(job_object_helper, 42);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_043: [ If job_object_helper is NULL, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_cpu_with_NULL_job_object_helper)
{
    // arrange

    // act
    int result = job_object_helper_limit_cpu(NULL, 42);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);

}

/*Tests_SRS_JOB_OBJECT_HELPER_18_044: [ If percent_cpu is 0, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_045: [ If percent_cpu is greater than 100, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_cpu_with_invalid_percent_cpu)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();

    uint32_t invalid_values[] = {0, 101, 143};

    for (int i=0; i < sizeof(invalid_values)/sizeof(invalid_values[0]); i++)
    {
        // act
        int result = job_object_helper_limit_cpu(job_object_helper, invalid_values[i]);

        // assert
        ASSERT_ARE_NOT_EQUAL(int, 0, result);
        // nothing is expected, nothing should be done.
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_047: [ job_object_helper_limit_cpu shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_050: [ job_object_helper_limit_cpu shall succeed and return 0. ]*/
TEST_FUNCTION(job_object_helper_limit_cpu_succeeds)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    setup_job_object_helper_limit_cpu_expectations();

    // act
    int result = job_object_helper_limit_cpu(job_object_helper, 42);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(unsigned_long, JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, captured_cpu_rate_control_information.ControlFlags);
    ASSERT_ARE_EQUAL(size_t, 4200, captured_cpu_rate_control_information.CpuRate);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_049: [ If there are any failures, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_cpu_fails)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();
    setup_job_object_helper_limit_cpu_expectations();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = job_object_helper_limit_cpu(job_object_helper, 42);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, 0, result);
        }
    }

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
}


/*Tests_SRS_JOB_OBJECT_HELPER_19_013: [ If percent_cpu is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_012: [ If percent_physical_memory is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_014: [ If percent_cpu and percent_physical_memory are 0 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_test_limits)
{
    // arrange
    int cpu_limits[4] = {0, 20, 101, 230 };
    int memory_limits[4] = {0, 242, 40, 230};
    ASSERT_ARE_EQUAL(int, MU_COUNT_ARRAY_ITEMS(cpu_limits), MU_COUNT_ARRAY_ITEMS(memory_limits), "cpu_limits and memory_limits must have the same number of items");

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        umock_c_reset_all_calls();
        // act
        THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("job_name", cpu_limits[i], memory_limits[i]);
        // assert
        ASSERT_IS_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }
}

/*Tests_SRS_JOB_OBJECT_HELPER_19_002: [ job_object_helper_set_job_limits_to_current_process shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes.]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_003: [ If percent_cpu is not 0 then `job_object_helper_set_job_limits_to_current_process` shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_004: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb.] */
/*Tests_SRS_JOB_OBJECT_HELPER_19_005: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_007: [ job_object_helper_set_job_limits_to_current_process shall call AssignProcessToJobObject with hJob set to job_object_helper->job_object and hProcess set to GetCurrentProcess() to assign the current process to the new job object.] */
/*Tests_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_set_job_limits_to_current_process shall succeed and return JOB_OBJECT_HELPER. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_succeeds)
{
    // arrange
    int cpu_limits[3] = { 50, 20, 0 };
    int memory_limits[3] = { 50, 0, 20 };

    ASSERT_ARE_EQUAL(int, MU_COUNT_ARRAY_ITEMS(cpu_limits), MU_COUNT_ARRAY_ITEMS(memory_limits), "cpu_limits and memory_limits must have the same number of items");

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        umock_c_reset_all_calls();
        setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
        if (cpu_limits[i] > 0)
        {
            setup_job_object_helper_limit_cpu_expectations();
        }
        if (memory_limits[i] > 0)
        {
            setup_job_object_helper_limit_memory_expectations();
        }
        setup_job_object_helper_set_job_limits_to_current_process_process_assign_expectations();

        // act
        THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("job_name", cpu_limits[i], memory_limits[i]);
        // assert
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
        ASSERT_IS_NOT_NULL(result);

        // cleanup
        THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result, NULL);
    }
}

/*Tests_SRS_JOB_OBJECT_HELPER_19_015: [ job_object_helper_set_job_limits_to_current_process shall allocate a JOB_OBJECT_HELPER object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_002: [ job_object_helper_set_job_limits_to_current_process shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes.]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_003: [ If percent_cpu is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_004: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_005: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_007: [ job_object_helper_set_job_limits_to_current_process shall call AssignProcessToJobObject with hJob set to job_object_helper->job_object and hProcess set to GetCurrentProcess() to assign the current process to the new job object.] */
/*Tests_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_fails)
{
    // arrange
    setup_job_object_helper_set_job_limits_to_current_process_expectations();
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG))
        .CallCannotFail();

    umock_c_negative_tests_snapshot();
    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);
            // act
            THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);
            // assert
            ASSERT_IS_NULL(result);
        }
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
