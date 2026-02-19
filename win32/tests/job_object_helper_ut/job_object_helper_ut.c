// Copyright(C) Microsoft Corporation.All rights reserved.


#include "job_object_helper_ut_pch.h"

#include "umock_c/umock_c_ENABLE_MOCKS.h" // ============================== ENABLE_MOCKS
#undef ENABLE_MOCKS_DECL
#include "umock_c/umock_c_prod.h"
    MOCKABLE_FUNCTION(, BOOL, mocked_AssignProcessToJobObject, HANDLE, hJob, HANDLE, hProcess);
#include "umock_c/umock_c_DISABLE_MOCKS.h" // ============================== DISABLE_MOCKS

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
MOCK_FUNCTION_END((HANDLE)-1)

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
    job_object_helper_deinit_for_test();
}

/*Tests_SRS_JOB_OBJECT_HELPER_18_033: [ job_object_helper_dispose shall call CloseHandle to close the handle to the job object. ]*/
TEST_FUNCTION(job_object_helper_dispose_succeeds)
{
    // arrange
    setup_job_object_helper_set_job_limits_to_current_process_expectations();

    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 1, 1);
    ASSERT_IS_NOT_NULL(job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // Release singleton ref (refcount 2->1), no dispose expected
    job_object_helper_deinit_for_test();
    umock_c_reset_all_calls();

    // Expect dispose (CloseHandle on job object) + free when last ref is released
    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act - release the last ref, triggering dispose
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);

    // assert
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
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
/*Tests_SRS_JOB_OBJECT_HELPER_19_003: [ If percent_cpu is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_004: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb.] */
/*Tests_SRS_JOB_OBJECT_HELPER_19_005: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_006: [ job_object_helper_set_job_limits_to_current_process shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_007: [ job_object_helper_set_job_limits_to_current_process shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_set_job_limits_to_current_process shall succeed and return a JOB_OBJECT_HELPER object. ]*/
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
        ASSERT_IS_NOT_NULL(result);
        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

        // cleanup - release result, then deinit releases singleton which triggers dispose
        THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result, NULL);

        // Expect dispose (CloseHandle on job object) + free of THANDLE wrapper when singleton refcount reaches 0
        STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        job_object_helper_deinit_for_test();

        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_001: [ If percent_cpu is 100 and percent_physical_memory is 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object (100% CPU and 100% memory are effectively no limits). ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_with_100_cpu_and_100_memory_returns_NULL)
{
    // arrange

    // act
    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("job_name", 100, 100);

    // assert
    ASSERT_IS_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_002: [ If the process-level singleton job object has already been created with the same percent_cpu and percent_physical_memory values, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_singleton_reuses_with_same_params)
{
    // arrange - create singleton
    setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);

    THANDLE(JOB_OBJECT_HELPER) first_result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);
    ASSERT_IS_NOT_NULL(first_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    // act - call again with same params
    THANDLE(JOB_OBJECT_HELPER) second_result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);

    // assert - should reuse singleton without any new creation calls
    ASSERT_IS_NOT_NULL(second_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&first_result, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&second_result, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_003: [ If the process-level singleton job object has already been created with different percent_cpu or percent_physical_memory values, job_object_helper_set_job_limits_to_current_process shall log an error and return NULL. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_singleton_with_different_params_returns_NULL)
{
    // arrange - create singleton with (50, 50)
    setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);

    THANDLE(JOB_OBJECT_HELPER) first_result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);
    ASSERT_IS_NOT_NULL(first_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    // act - call with different params
    THANDLE(JOB_OBJECT_HELPER) second_result = job_object_helper_set_job_limits_to_current_process("job_name", 30, 30);

    // assert - should fail, no creation calls
    ASSERT_IS_NULL(second_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&first_result, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_005: [ job_object_helper_deinit_for_test shall release the singleton THANDLE(JOB_OBJECT_HELPER) and reset the stored parameters to zero. ]*/
TEST_FUNCTION(job_object_helper_deinit_for_test_resets_singleton)
{
    // arrange - create singleton with (50, 50)
    setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);

    THANDLE(JOB_OBJECT_HELPER) first_result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);
    ASSERT_IS_NOT_NULL(first_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&first_result, NULL);
    umock_c_reset_all_calls();

    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    job_object_helper_deinit_for_test();

    // assert - deinit released the singleton, verify cleanup calls
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // assert - verify a new singleton can be created (proving deinit reset the state)
    umock_c_reset_all_calls();
    setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    STRICT_EXPECTED_CALL(mocked_GetCurrentProcess())
        .CallCannotFail();
    STRICT_EXPECTED_CALL(mocked_AssignProcessToJobObject(IGNORED_ARG, IGNORED_ARG))
        .SetReturn(TRUE)
        .SetFailReturn(FALSE);

    THANDLE(JOB_OBJECT_HELPER) second_result = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);
    ASSERT_IS_NOT_NULL(second_result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&second_result, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_19_015: [ job_object_helper_set_job_limits_to_current_process shall allocate a JOB_OBJECT_HELPER object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_002: [ job_object_helper_set_job_limits_to_current_process shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes.]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_003: [ If percent_cpu is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_004: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_005: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_006: [ job_object_helper_set_job_limits_to_current_process shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_007: [ job_object_helper_set_job_limits_to_current_process shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
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
