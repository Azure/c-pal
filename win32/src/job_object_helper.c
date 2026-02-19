// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stdlib.h>

#include "windows.h"
#include "processthreadsapi.h"
#include "jobapi2.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"

#include "c_pal/job_object_helper.h"

#define MAX_CPU_PERCENT 100
#define MAX_MEMORY_PERCENT 100

typedef struct JOB_OBJECT_HELPER_TAG {
    HANDLE job_object;
} JOB_OBJECT_HELPER;

THANDLE_TYPE_DEFINE(JOB_OBJECT_HELPER);

/* Process-level singleton state to prevent Job Object accumulation.
   Windows Job Objects compound CPU rate limits multiplicatively when
   a process is assigned to multiple jobs. CloseHandle does NOT
   disassociate the process from the job. Since this function calls
   AssignProcessToJobObject on the current process, it must ensure
   that only one job object is ever created per process lifetime. */

typedef struct JOB_OBJECT_SINGLETON_STATE_TAG
{
    THANDLE(JOB_OBJECT_HELPER) job_object_helper;
    uint32_t percent_cpu;
    uint32_t percent_physical_memory;
} JOB_OBJECT_SINGLETON_STATE;

static JOB_OBJECT_SINGLETON_STATE job_object_singleton_state = { NULL, 0, 0 };

static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper)
{
    /*Codes_SRS_JOB_OBJECT_HELPER_18_033: [ job_object_helper_dispose shall call CloseHandle to close the handle to the job object. ]*/
    if (!CloseHandle(job_object_helper->job_object))
    {
        LogLastError("failure in CloseHandle(job_object_helper->job_object=%p)", job_object_helper->job_object);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory)
{
    THANDLE(JOB_OBJECT_HELPER) result = NULL;

    if (
        /*Codes_SRS_JOB_OBJECT_HELPER_19_013: [ If percent_cpu is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
        percent_cpu > MAX_CPU_PERCENT ||
        /*Codes_SRS_JOB_OBJECT_HELPER_19_012: [ If percent_physical_memory is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
        percent_physical_memory > MAX_MEMORY_PERCENT ||
        /*Codes_SRS_JOB_OBJECT_HELPER_19_014: [ If percent_cpu and percent_physical_memory are 0 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
        (percent_cpu == 0 && percent_physical_memory == 0))
    {
        LogError("Invalid arguments: job_name=%s, percent_cpu=%" PRIu32 ", percent_physical_memory=%" PRIu32 "", MU_P_OR_NULL(job_name), percent_cpu, percent_physical_memory);
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_001: [ If percent_cpu is 100 and percent_physical_memory is 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object (100% CPU and 100% memory are effectively no limits). ]*/
        if (percent_cpu == MAX_CPU_PERCENT && percent_physical_memory == MAX_MEMORY_PERCENT)
        {
            LogInfo("percent_cpu is 100 and percent_physical_memory is 100, no job object needed (100%% CPU and 100%% memory are effectively no limits)");
        }
        else
        {
            if (job_object_singleton_state.job_object_helper != NULL)
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_003: [ If the process-level singleton job object has already been created with different percent_cpu or percent_physical_memory values, job_object_helper_set_job_limits_to_current_process shall log an error and return NULL. ]*/
                if (job_object_singleton_state.percent_cpu != percent_cpu || job_object_singleton_state.percent_physical_memory != percent_physical_memory)
                {
                    LogError("Job object limits have changed (cpu: %" PRIu32 "->%" PRIu32 ", memory: %" PRIu32 "->%" PRIu32 "). "
                        "not reconfiguring Job Object after process assignment. Returning NULL.",
                        job_object_singleton_state.percent_cpu, percent_cpu, job_object_singleton_state.percent_physical_memory, percent_physical_memory);
                }
                else
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_88_002: [ If the process-level singleton job object has already been created with the same percent_cpu and percent_physical_memory values, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
                    LogInfo("Reusing existing process-level singleton Job Object (percent_cpu=%" PRIu32 ", percent_physical_memory=%" PRIu32 ")",
                        percent_cpu, percent_physical_memory);
                    THANDLE_INITIALIZE(JOB_OBJECT_HELPER)(&result, job_object_singleton_state.job_object_helper);
                }
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_19_015: [ job_object_helper_set_job_limits_to_current_process shall allocate a JOB_OBJECT_HELPER object. ]*/
                JOB_OBJECT_HELPER* job_object_helper = THANDLE_MALLOC(JOB_OBJECT_HELPER)(job_object_helper_dispose);
                if (job_object_helper == NULL)
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
                    LogError("failure in THANDLE_MALLOC(JOB_OBJECT_HELPER)(job_object_helper_dispose=%p)", job_object_helper_dispose);
                }
                else
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_002: [ job_object_helper_set_job_limits_to_current_process shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes. ] */
                    job_object_helper->job_object = CreateJobObjectA(NULL, job_name);
                    if (job_object_helper->job_object == NULL)
                    {
                        /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ] */
                        LogLastError("failure in CreateJobObjectA(lpJobAttributes=NULL, job_name=%s)", MU_P_OR_NULL(job_name));
                    }
                    else
                    {
                        bool failed = false;
                        /*Codes_SRS_JOB_OBJECT_HELPER_19_003: [ If percent_cpu is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
                        if (percent_cpu != 0)
                        {
                            JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_rate_control_information = { 0 };
                            cpu_rate_control_information.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
                            cpu_rate_control_information.CpuRate = percent_cpu * 100;
                            if (!SetInformationJobObject(job_object_helper->job_object, JobObjectCpuRateControlInformation, &cpu_rate_control_information, sizeof(cpu_rate_control_information)))
                            {
                                /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ] */
                                LogLastError("failure in SetInformationJobObject(job_object=%p, JobObjectCpuRateControlInformation, &cpu_rate_control_information=%p, sizeof(cpu_rate_control_information)=%zu)",
                                    job_object_helper->job_object, &cpu_rate_control_information, sizeof(cpu_rate_control_information));
                                failed = true;
                            }
                        }

                        if (failed)
                        {
                            /* Error already logged in previous check of SetInformationJobObject */
                        }
                        else
                        {
                            if (percent_physical_memory != 0)
                            {
                                /*Codes_SRS_JOB_OBJECT_HELPER_19_004: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
                                MEMORYSTATUSEX memory_status_ex;
                                memory_status_ex.dwLength = sizeof(memory_status_ex);
                                if (!GlobalMemoryStatusEx(&memory_status_ex))
                                {
                                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
                                    LogLastError("failure in GlobalMemoryStatusEx(&memory_status_ex=%p)", &memory_status_ex);
                                    failed = true;
                                }
                                else
                                {
                                    /*Codes_SRS_JOB_OBJECT_HELPER_19_005: [ If percent_physical_memory is not 0 then job_object_helper_set_job_limits_to_current_process shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
                                    JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information = { 0 };
                                    extended_limit_information.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY | JOB_OBJECT_LIMIT_PROCESS_MEMORY;
                                    extended_limit_information.JobMemoryLimit = percent_physical_memory * memory_status_ex.ullTotalPhys / 100;
                                    extended_limit_information.ProcessMemoryLimit = percent_physical_memory * memory_status_ex.ullTotalPhys / 100;
                                    if (!SetInformationJobObject(job_object_helper->job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information)))
                                    {
                                        /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL.] */
                                        LogLastError("failure in SetInformationJobObject(job_object=%p, JobObjectExtendedLimitInformation, &extended_limit_information=%p, sizeof(extended_limit_information)=%zu)",
                                            job_object_helper->job_object, &extended_limit_information, sizeof(extended_limit_information));
                                        failed = true;
                                    }
                                }
                            }

                            if (failed)
                            {
                                /* Error already logged in previous check of SetInformationJobObject */
                            }
                            else
                            {
                                /*Codes_SRS_JOB_OBJECT_HELPER_19_006: [ job_object_helper_set_job_limits_to_current_process shall call GetCurrentProcess to get the current process handle. ]*/
                                HANDLE current_process = GetCurrentProcess();
                                /*Codes_SRS_JOB_OBJECT_HELPER_19_007: [ job_object_helper_set_job_limits_to_current_process shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
                                BOOL assign_process_to_job_object_result = AssignProcessToJobObject(job_object_helper->job_object, current_process);
                                if (!assign_process_to_job_object_result)
                                {
                                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
                                    LogLastError("failure in AssignProcessToJobObject(job_object=%p, current_process=%p)", job_object_helper->job_object, current_process);
                                }
                                else
                                {
                                    /*Codes_SRS_JOB_OBJECT_HELPER_88_004: [ On success, job_object_helper_set_job_limits_to_current_process shall store the THANDLE(JOB_OBJECT_HELPER) and the percent_cpu and percent_physical_memory values in static variables for the singleton pattern. ]*/
                                    THANDLE_INITIALIZE_MOVE(JOB_OBJECT_HELPER)(&result, &job_object_helper);
                                    THANDLE_INITIALIZE(JOB_OBJECT_HELPER)(&job_object_singleton_state.job_object_helper, result);
                                    job_object_singleton_state.percent_cpu = percent_cpu;
                                    job_object_singleton_state.percent_physical_memory = percent_physical_memory;
                                    /*Codes_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_set_job_limits_to_current_process shall succeed and return a JOB_OBJECT_HELPER object. ]*/
                                    goto all_ok;
                                }
                            }
                        }
                        if (!CloseHandle(job_object_helper->job_object))
                        {
                            LogLastError("failure in CloseHandle(job_object_helper->job_object=%p)", job_object_helper->job_object);
                        }
                    }
                    THANDLE_FREE(JOB_OBJECT_HELPER)(job_object_helper);
                }
            }
        }
    }

all_ok:
    /*Codes_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_set_job_limits_to_current_process shall succeed and return a JOB_OBJECT_HELPER object. ]*/
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test)
{
    /*Codes_SRS_JOB_OBJECT_HELPER_88_005: [ job_object_helper_deinit_for_test shall release the singleton THANDLE(JOB_OBJECT_HELPER) and reset the stored parameters to zero. ]*/
    LogWarning("job_object_helper_deinit_for_test called - this should only be used for test cleanup");
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_singleton_state.job_object_helper, NULL);
    job_object_singleton_state.percent_cpu = 0;
    job_object_singleton_state.percent_physical_memory = 0;
}
