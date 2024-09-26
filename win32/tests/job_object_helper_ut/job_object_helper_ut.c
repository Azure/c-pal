// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "windows.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_windows.h"

#define ENABLE_MOCKS
    #include "c_pal/gballoc_hl.h"
    #include "c_pal/gballoc_hl_redirect.h"

    MOCKABLE_FUNCTION(, HANDLE, mocked_CreateJobObject, LPSECURITY_ATTRIBUTES, lpJobAttributes, LPCSTR, lpName);
    MOCKABLE_FUNCTION(, HANDLE, mocked_GetCurrentProcess)
    MOCKABLE_FUNCTION(, BOOL, mocked_AssignProcessToJobObject, HANDLE, hJob, HANDLE, hProcess);
    MOCKABLE_FUNCTION(, BOOL, mocked_CloseHandle, HANDLE, hObject);

#undef ENABLE_MOCKS

#include "real_gballoc_hl.h"
#include "c_pal/job_object_helper.h"

MU_DEFINE_ENUM_STRINGS(UMOCK_C_ERROR_CODE, UMOCK_C_ERROR_CODE_VALUES)

static void on_umock_c_error(UMOCK_C_ERROR_CODE error_code)
{
    ASSERT_FAIL("umock_c reported error :%" PRI_MU_ENUM "", MU_ENUM_VALUE(UMOCK_C_ERROR_CODE, error_code));
}

#define test_percent_cpu 42
#define test_percent_physical_memory 89
#define test_job_object (HANDLE)0x42001
#define test_process (HANDLE)0x42002
#define test_total_physical_memory 20480

MOCK_FUNCTION_WITH_CODE(, BOOL, mocked_GlobalMemoryStatusEx, LPMEMORYSTATUSEX, lpBuffer)
{
    ASSERT_IS_NOT_NULL(lpBuffer);
    ASSERT_ARE_EQUAL(size_t, lpBuffer->dwLength, sizeof(MEMORYSTATUSEX));
    memset(lpBuffer, 0, sizeof(MEMORYSTATUSEX));
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
        memcpy((void*)&captured_extended_limit_information, lpJobObjectInformation, cbJobObjectInformationLength);
    }
    else if (JobObjectInformationClass == JobObjectCpuRateControlInformation)
    {
        memcpy((void*)&captured_cpu_rate_control_information, lpJobObjectInformation, cbJobObjectInformationLength);
    }
    else
    {
        ASSERT_FAIL();
    }
}
MOCK_FUNCTION_END(TRUE)

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    int result;

    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL));

    result = umock_c_init(on_umock_c_error);
    ASSERT_ARE_EQUAL(int, 0, result, "umock_c_init failed");

    result = umocktypes_windows_register_types();
    ASSERT_ARE_EQUAL(int, 0, result, "umocktypes_windows_register_types failed");

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

/*Tests_SRS_JOB_OBJECT_HELPER_18_001: [ If percent_physical_memory is 0, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_resources_called_with_0_memory)
{
    // arrange

    // act
    int result = job_object_helper_limit_resources(0, test_percent_cpu);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_002: [ If percent_cpu is 0, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_resources_called_with_0_cpu)
{
    // arrange

    // act
    int result = job_object_helper_limit_resources(test_percent_physical_memory, 0);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_004: [ If percent_physical_memory is greater than 100, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_resources_called_with_101_memory)
{
    // arrange

    // act
    int result = job_object_helper_limit_resources(101, test_percent_cpu);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_005: [ If percent_cpu is greater than 100, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_resources_called_with_101_cpu)
{
    // arrange

    // act
    int result = job_object_helper_limit_resources(test_percent_physical_memory, 101);

    // assert
    ASSERT_ARE_NOT_EQUAL(int, 0, result);
}

static void setup_job_object_helper_limit_resources_expected_calls()
{
    STRICT_EXPECTED_CALL(mocked_CreateJobObject(NULL,NULL))
        .SetReturn(test_job_object)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .SetReturn(test_process)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(test_job_object, test_process))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_GlobalMemoryStatusEx(IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_SetInformationJobObject(test_job_object, JobObjectExtendedLimitInformation, IGNORED_ARG, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_SetInformationJobObject(test_job_object, JobObjectCpuRateControlInformation, IGNORED_ARG, sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION)))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_process))
        .SetReturn(TRUE)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_job_object))
        .SetReturn(TRUE)
        .CallCannotFail();
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_006: [ job_object_helper_limit_resources shall call CreateJobObject to create a new job object passing NULL for both lpJobAttributes and lpName. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_007: [ job_object_helper_limit_resources shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_008: [ job_object_helper_limit_resources shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_009: [ job_object_helper_limit_resources shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_010: [ job_object_helper_limit_resources shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_011: [ job_object_helper_limit_resources shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_012: [ job_object_helper_limit_resources shall call CloseHandle to close the handle of the current process. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_013: [ job_object_helper_limit_resources shall call CloseHandle to close the handle to the job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_014: [ job_object_helper_limit_resources shall succeed and return 0. ]*/
TEST_FUNCTION(job_object_helper_limit_resources_succeeds)
{
    // arrange
    setup_job_object_helper_limit_resources_expected_calls();

    // act
    int result = job_object_helper_limit_resources(test_percent_physical_memory, test_percent_cpu);

    // assert
    ASSERT_ARE_EQUAL(int, 0, result);

    ASSERT_ARE_EQUAL(int, captured_extended_limit_information.BasicLimitInformation.LimitFlags, JOB_OBJECT_LIMIT_JOB_MEMORY);
    ASSERT_ARE_EQUAL(int, captured_extended_limit_information.JobMemoryLimit, test_total_physical_memory * test_percent_physical_memory / 100);

    ASSERT_ARE_EQUAL(int, captured_cpu_rate_control_information.ControlFlags, JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP);
    ASSERT_ARE_EQUAL(int, captured_cpu_rate_control_information.CpuRate, test_percent_cpu * 100);

    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_limit_resources_fails)
{
    // arrange
    setup_job_object_helper_limit_resources_expected_calls();
    umock_c_negative_tests_snapshot();

    for (size_t i = 0; i < umock_c_negative_tests_call_count(); i++)
    {
        if (umock_c_negative_tests_can_call_fail(i))
        {
            umock_c_negative_tests_reset();
            umock_c_negative_tests_fail_call(i);

            // act
            int result = job_object_helper_limit_resources(test_percent_physical_memory, test_percent_cpu);

            // assert
            ASSERT_ARE_NOT_EQUAL(int, result, 0, "On failed call %zu", i);
        }
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
