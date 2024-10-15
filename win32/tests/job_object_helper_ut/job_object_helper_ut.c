// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"
#include "umock_c/umock_c.h"
#include "umock_c/umock_c_negative_tests.h"
#include "umock_c/umocktypes_windows.h"

#define ENABLE_MOCKS
    #include "c_pal/interlocked.h"
    #include "c_pal/interlocked_hl.h"
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
#define test_process_handle (HANDLE)0x42002
#define test_total_physical_memory 20480

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
    STRICT_EXPECTED_CALL(interlocked_exchange(IGNORED_ARG, 1))
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_GlobalMemoryStatusEx(IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_CreateJobObject(NULL, NULL))
        .SetReturn(test_job_object)
        .SetFailReturn(NULL);
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .SetReturn(test_process_handle)
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(test_job_object, test_process_handle))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_process_handle))
        .SetReturn(TRUE)
        .CallCannotFail();
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, real_gballoc_hl_init(NULL, NULL), "real_gballoc_hl_init failed");
    ASSERT_ARE_EQUAL(int, 0, umock_c_init(on_umock_c_error), "umock_c_init failed");
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

/*Tests_SRS_JOB_OBJECT_HELPER_18_035: [ If job_object_helper is NULL, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_036: [ If percent_physical_memory is 0, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_037: [ If percent_physical_memory is greater than 100, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_039: [ job_object_helper_limit_memory shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_041: [ If there are any failures, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_042: [ job_object_helper_limit_memory shall succeed and return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_043: [ If job_object_helper is NULL, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_044: [ If percent_cpu is 0, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_045: [ If percent_cpu is greater than 100, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_047: [ job_object_helper_limit_cpu shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_049: [ If there are any failures, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_050: [ job_object_helper_limit_cpu shall succeed and return 0. ]*/


/*Tests_SRS_JOB_OBJECT_HELPER_18_016: [ job_object_helper_create shall allocate a JOB_OBJECT_HELPER object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_023: [ job_object_helper_create shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_024: [ job_object_helper_create shall call CreateJobObject to create a new job object passing NULL for both lpJobAttributes and lpName. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_025: [ job_object_helper_create shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_026: [ job_object_helper_create shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_027: [ job_object_helper_create shall call CloseHandle to close the handle of the current process. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_18_019: [ job_object_helper_create shall succeed and return the JOB_OBJECT_HELPER object. ]*/
TEST_FUNCTION(test_job_object_helper_create_succeeds)
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
TEST_FUNCTION(test_job_object_helper_create_fails)
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
TEST_FUNCTION(test_job_object_helper_dispose_succeeds)
{
    // arrange
    setup_job_object_helper_create_expectations();
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_create();
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(interlocked_decrement(IGNORED_ARG));
    STRICT_EXPECTED_CALL(mocked_CloseHandle(test_job_object));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
