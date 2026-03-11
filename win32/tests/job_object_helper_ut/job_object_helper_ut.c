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

static THANDLE(JOB_OBJECT_HELPER) create_job_object_helper_singleton(uint32_t cpu, uint32_t memory)
{
    setup_job_object_helper_set_job_limits_to_current_process_createObjectA_expectations();
    if (cpu != 0)
    {
        setup_job_object_helper_limit_cpu_expectations();
    }
    if (memory != 0)
    {
        setup_job_object_helper_limit_memory_expectations();
    }
    setup_job_object_helper_set_job_limits_to_current_process_process_assign_expectations();

    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("job_name", cpu, memory);
    ASSERT_IS_NOT_NULL(result);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();
    return result;
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
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = create_job_object_helper_singleton(1, 1);

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

/*Tests_SRS_JOB_OBJECT_HELPER_19_001: [ If percent_cpu is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_034: [ If percent_physical_memory is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_004: [ If percent_cpu is 0 or greater than 100, internal_job_object_helper_set_cpu_limit shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_010: [ If percent_physical_memory is 0 or greater than 100, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_test_limits)
{
    // arrange
    int cpu_limits[3] = { 20, 101, 230 };
    int memory_limits[3] = { 242, 40, 230 };
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

/*Tests_SRS_JOB_OBJECT_HELPER_88_031: [ job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_create to create a new job object and assign it to the current process. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_035: [ internal_job_object_helper_create shall allocate a JOB_OBJECT_HELPER object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_036: [ internal_job_object_helper_create shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes.]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_037: [ If percent_cpu is not 0, internal_job_object_helper_create shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_005: [ internal_job_object_helper_set_cpu_limit shall set ControlFlags to JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, and CpuRate to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_006: [ internal_job_object_helper_set_cpu_limit shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and the JOBOBJECT_CPU_RATE_CONTROL_INFORMATION. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_042: [ On success, internal_job_object_helper_set_cpu_limit shall store percent_cpu in the singleton state. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_038: [ If percent_physical_memory is not 0, internal_job_object_helper_create shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_011: [ internal_job_object_helper_set_memory_limit shall call GlobalMemoryStatusEx to get the total amount of physical memory. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_012: [ internal_job_object_helper_set_memory_limit shall set JobMemoryLimit and ProcessMemoryLimit to percent_physical_memory percent of the physical memory and call SetInformationJobObject with JobObjectExtendedLimitInformation. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_043: [ On success, internal_job_object_helper_set_memory_limit shall store percent_physical_memory in the singleton state. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_039: [ internal_job_object_helper_create shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_008: [ internal_job_object_helper_create shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_008: [ internal_job_object_helper_set_cpu_limit shall succeed and return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_014: [ internal_job_object_helper_set_memory_limit shall succeed and return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_024: [ On success, internal_job_object_helper_create shall store the THANDLE(JOB_OBJECT_HELPER) in the process-level singleton state. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_033: [ internal_job_object_helper_create shall succeed and return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_helper_set_job_limits_to_current_process shall succeed and return a JOB_OBJECT_HELPER object. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_succeeds)
{
    // arrange
    int cpu_limits[3] = { 50, 20, 0 };
    int memory_limits[3] = { 50, 0, 20 };

    ASSERT_ARE_EQUAL(int, MU_COUNT_ARRAY_ITEMS(cpu_limits), MU_COUNT_ARRAY_ITEMS(memory_limits), "cpu_limits and memory_limits must have the same number of items");

    for (int i = 0; i < MU_COUNT_ARRAY_ITEMS(cpu_limits); ++i)
    {
        // act
        THANDLE(JOB_OBJECT_HELPER) result = create_job_object_helper_singleton(cpu_limits[i], memory_limits[i]);

        // assert
        THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result, NULL);

        // Expect dispose (CloseHandle on job object) + free of THANDLE wrapper when singleton refcount reaches 0
        STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
        STRICT_EXPECTED_CALL(free(IGNORED_ARG));
        job_object_helper_deinit_for_test();

        ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    }
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_040: [ If percent_cpu is 0 and percent_physical_memory is 0 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_023: [ If job_object_singleton_state.job_object_helper is NULL and both percent_cpu and percent_physical_memory are 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_NULL_when_no_effective_limits)
{
    // arrange
    int cpu_limits[2] = { 0, 100 };
    int memory_limits[2] = { 0, 100 };

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

/*Tests_SRS_JOB_OBJECT_HELPER_88_030: [ If job_object_singleton_state.job_object_helper is not NULL, job_object_helper_set_job_limits_to_current_process shall not create a new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_002: [ If job_object_singleton_state.job_object_helper is not NULL, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_reconfigure to apply the limits to the existing job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_003: [ If percent_cpu is not 0, internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_009: [ If percent_physical_memory is not 0, internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_020: [ On successful reconfiguration, internal_job_object_helper_reconfigure shall return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_021: [ If internal_job_object_helper_reconfigure returns 0, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_reconfigures_with_same_params)
{
    // arrange - create singleton
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // expect reconfigure calls (idempotent: same values applied again)
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();

    // act - call again with same params
    THANDLE(JOB_OBJECT_HELPER) reused_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 50, 50);

    // assert - should reuse singleton (same pointer)
    ASSERT_IS_NOT_NULL(reused_job_object_helper);
    ASSERT_ARE_EQUAL(void_ptr, initial_job_object_helper, reused_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reused_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_041: [ During reconfiguration, if percent_cpu is 0 and the job_object_singleton_state.percent_cpu is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_NULL_when_reconfigure_cpu_from_nonzero_to_zero)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // act - reconfigure cpu from 50->0 should fail (SRS_88_041)
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 0, 50);

    // assert - should fail because percent_cpu was non-zero (50) and now being set to 0
    ASSERT_IS_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_046: [ During reconfiguration, if percent_physical_memory is 0 and the job_object_singleton_state.percent_memory is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_NULL_when_reconfigure_memory_from_nonzero_to_zero)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // act - reconfigure memory from 50->0 should fail (SRS_88_041)
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 50, 0);

    // assert - should fail because percent_memory was non-zero (50) and now being set to 0
    ASSERT_IS_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_046: [ During reconfiguration, if percent_physical_memory is 0 and the job_object_singleton_state.percent_memory is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(when_created_with_cpu_0_memory_50_then_reconfigure_cpu_50_memory_0_fails)
{
    // arrange - create singleton with cpu=0, memory=50
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(0, 50);

    // act - reconfigure to cpu=50, memory=0. memory was 50 (non-zero) so setting to 0 should fail
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 50, 0);

    // assert - should fail because percent_memory was non-zero (50) and now being set to 0
    ASSERT_IS_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_042: [ On success, internal_job_object_helper_set_cpu_limit shall store percent_cpu in the singleton state. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_041: [ During reconfiguration, if percent_cpu is 0 and the job_object_singleton_state.percent_cpu is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(when_reconfigure_sets_cpu_then_state_is_updated_and_subsequent_zero_cpu_fails)
{
    // arrange - create singleton with cpu=0, memory=50 (only memory limit)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(0, 50);

    // Reconfigure to (30, 50) — this should store percent_cpu=30 in singleton state
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 30, 50);
    ASSERT_IS_NOT_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // act - try to reconfigure cpu back to 0. If state was updated, percent_cpu is now 30
    // and this should fail (SRS_88_041). If state was NOT updated, percent_cpu would still
    // be 0 and this would incorrectly succeed.
    THANDLE(JOB_OBJECT_HELPER) zero_cpu_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 0, 50);

    // assert - must fail because percent_cpu was stored as 30 during reconfiguration
    ASSERT_IS_NULL(zero_cpu_job_object_helper);

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reconfigured_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_043: [ On success, internal_job_object_helper_set_memory_limit shall store percent_physical_memory in the singleton state. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_046: [ During reconfiguration, if percent_physical_memory is 0 and the job_object_singleton_state.percent_memory is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
TEST_FUNCTION(when_reconfigure_sets_memory_then_state_is_updated_and_subsequent_zero_memory_fails)
{
    // arrange - create singleton with cpu=50, memory=0 (only CPU limit)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 0);

    // Reconfigure to (50, 30) — this should store percent_memory=30 in singleton state
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 50, 30);
    ASSERT_IS_NOT_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // act - try to reconfigure memory back to 0. If state was updated, percent_memory is
    // now 30 and this should fail (SRS_88_041). If state was NOT updated, percent_memory
    // would still be 0 and this would incorrectly succeed.
    THANDLE(JOB_OBJECT_HELPER) zero_memory_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 50, 0);

    // assert - must fail because percent_memory was stored as 30 during reconfiguration
    ASSERT_IS_NULL(zero_memory_job_object_helper);

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reconfigured_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_030: [ If job_object_singleton_state.job_object_helper is not NULL, job_object_helper_set_job_limits_to_current_process shall not create a new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_002: [ If job_object_singleton_state.job_object_helper is not NULL, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_reconfigure to apply the limits to the existing job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_003: [ If percent_cpu is not 0, internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_009: [ If percent_physical_memory is not 0, internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_020: [ On successful reconfiguration, internal_job_object_helper_reconfigure shall return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_021: [ If internal_job_object_helper_reconfigure returns 0, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_reconfigures_when_params_change)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // expect reconfigure calls: CPU limit change + memory limit change
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();

    // act - reconfigure to (30, 30)
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 30, 30);

    // assert
    ASSERT_IS_NOT_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(void_ptr, initial_job_object_helper, reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // verify the captured CPU rate matches the new value (30 * 100 = 3000)
    ASSERT_ARE_EQUAL(uint32_t, 3000, captured_cpu_rate_control_information.CpuRate);

    // verify the captured memory limit matches the new value (30% of test_total_physical_memory = 6144)
    ASSERT_ARE_EQUAL(size_t, (size_t)(30 * test_total_physical_memory / 100), captured_extended_limit_information.JobMemoryLimit);
    ASSERT_ARE_EQUAL(size_t, (size_t)(30 * test_total_physical_memory / 100), captured_extended_limit_information.ProcessMemoryLimit);

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reconfigured_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_003: [ If percent_cpu is not 0, internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_009: [ If percent_physical_memory is not 0, internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_020: [ On successful reconfiguration, internal_job_object_helper_reconfigure shall return 0. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_021: [ If internal_job_object_helper_reconfigure returns 0, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_reconfigures_to_100_100)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // expect reconfigure calls: CPU limit change + memory limit change
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();

    // act - reconfigure to (100, 100) — effectively no limits but allowed for existing singleton
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 100, 100);

    // assert
    ASSERT_IS_NOT_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(void_ptr, initial_job_object_helper, reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // verify CpuRate=10000 (100 * 100) and ControlFlags have ENABLE+HARD_CAP
    ASSERT_ARE_EQUAL(uint32_t, 10000, captured_cpu_rate_control_information.CpuRate);
    ASSERT_ARE_EQUAL(uint32_t, JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, captured_cpu_rate_control_information.ControlFlags);

    // verify memory limit is 100% of test_total_physical_memory
    ASSERT_ARE_EQUAL(size_t, (size_t)test_total_physical_memory, captured_extended_limit_information.JobMemoryLimit);
    ASSERT_ARE_EQUAL(size_t, (size_t)test_total_physical_memory, captured_extended_limit_information.ProcessMemoryLimit);

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&reconfigured_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_007: [ If SetInformationJobObject fails, internal_job_object_helper_set_cpu_limit shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_017: [ If there are any failures, internal_job_object_helper_reconfigure shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_NULL_when_cpu_reconfigure_fails)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // expect CPU SetInformationJobObject to fail
    STRICT_EXPECTED_CALL(mocked_SetInformationJobObject(IGNORED_ARG, JobObjectCpuRateControlInformation, IGNORED_ARG, sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION)))
        .SetReturn(FALSE);

    // act - reconfigure to (30, 30) but CPU reconfigure fails
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 30, 30);

    // assert
    ASSERT_IS_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_013: [ If there are any failures, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_017: [ If there are any failures, internal_job_object_helper_reconfigure shall fail and return a non-zero value. ]*/
TEST_FUNCTION(job_object_helper_set_job_limits_to_current_process_returns_NULL_when_memory_reconfigure_fails)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);

    // expect CPU reconfigure succeeds, but memory GlobalMemoryStatusEx fails
    setup_job_object_helper_limit_cpu_expectations();
    STRICT_EXPECTED_CALL(mocked_GlobalMemoryStatusEx(IGNORED_ARG))
        .SetReturn(FALSE);

    // act - reconfigure to (30, 30) but memory reconfigure fails
    THANDLE(JOB_OBJECT_HELPER) reconfigured_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 30, 30);

    // assert - returns NULL
    ASSERT_IS_NULL(reconfigured_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());
    umock_c_reset_all_calls();

    // assert - verify that after partial failure, a subsequent reconfigure still succeeds
    // A subsequent call with (30, 50) reconfigures and succeeds
    setup_job_object_helper_limit_cpu_expectations();
    setup_job_object_helper_limit_memory_expectations();
    THANDLE(JOB_OBJECT_HELPER) subsequent_job_object_helper = job_object_helper_set_job_limits_to_current_process("job_name", 30, 50);
    ASSERT_IS_NOT_NULL(subsequent_job_object_helper);
    ASSERT_ARE_EQUAL(void_ptr, initial_job_object_helper, subsequent_job_object_helper);
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&subsequent_job_object_helper, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_027: [ job_object_helper_deinit_for_test shall release the singleton THANDLE(JOB_OBJECT_HELPER) by assigning it to NULL. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_044: [ job_object_helper_deinit_for_test shall reset percent_cpu to 0 in the singleton state. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_045: [ job_object_helper_deinit_for_test shall reset percent_memory to 0 in the singleton state. ]*/
TEST_FUNCTION(job_object_helper_deinit_for_test_resets_singleton)
{
    // arrange - create singleton with (50, 50)
    THANDLE(JOB_OBJECT_HELPER) initial_job_object_helper = create_job_object_helper_singleton(50, 50);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&initial_job_object_helper, NULL);

    STRICT_EXPECTED_CALL(mocked_CloseHandle(IGNORED_ARG));
    STRICT_EXPECTED_CALL(free(IGNORED_ARG));

    // act
    job_object_helper_deinit_for_test();

    // assert - deinit released the singleton, verify cleanup calls
    ASSERT_ARE_EQUAL(char_ptr, umock_c_get_expected_calls(), umock_c_get_actual_calls());

    // assert - verify a new singleton can be created (proving deinit reset the state)
    THANDLE(JOB_OBJECT_HELPER) new_job_object_helper = create_job_object_helper_singleton(50, 50);

    // cleanup
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&new_job_object_helper, NULL);
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_031: [ job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_create to create a new job object and assign it to the current process. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_035: [ internal_job_object_helper_create shall allocate a JOB_OBJECT_HELPER object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_036: [ internal_job_object_helper_create shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes.]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_037: [ If percent_cpu is not 0, internal_job_object_helper_create shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_005: [ internal_job_object_helper_set_cpu_limit shall set ControlFlags to JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, and CpuRate to percent_cpu times 100. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_006: [ internal_job_object_helper_set_cpu_limit shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and the JOBOBJECT_CPU_RATE_CONTROL_INFORMATION. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_007: [ If SetInformationJobObject fails, internal_job_object_helper_set_cpu_limit shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_038: [ If percent_physical_memory is not 0, internal_job_object_helper_create shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_011: [ internal_job_object_helper_set_memory_limit shall call GlobalMemoryStatusEx to get the total amount of physical memory. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_012: [ internal_job_object_helper_set_memory_limit shall set JobMemoryLimit and ProcessMemoryLimit to percent_physical_memory percent of the physical memory and call SetInformationJobObject with JobObjectExtendedLimitInformation. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_013: [ If there are any failures, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_039: [ internal_job_object_helper_create shall call GetCurrentProcess to get the current process handle. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_19_008: [ internal_job_object_helper_create shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_032: [ If there are any failures, internal_job_object_helper_create shall fail and return a non-zero value. ]*/
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
