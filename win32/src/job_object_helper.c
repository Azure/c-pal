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
} JOB_OBJECT_SINGLETON_STATE;

static JOB_OBJECT_SINGLETON_STATE job_object_singleton_state = { NULL };

static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper)
{
    /*Codes_SRS_JOB_OBJECT_HELPER_18_033: [ job_object_helper_dispose shall call CloseHandle to close the handle to the job object. ]*/
    if (!CloseHandle(job_object_helper->job_object))
    {
        LogLastError("failure in CloseHandle(job_object_helper->job_object=%p)", job_object_helper->job_object);
    }
}

static int internal_job_object_helper_set_cpu_limit(HANDLE job_object, uint32_t percent_cpu)
{
    int result;

    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_rate_control_information = { 0 };
    if (percent_cpu == JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_004: [ If percent_cpu is JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL, internal_job_object_helper_set_cpu_limit shall call SetInformationJobObject passing JobObjectCpuRateControlInformation with ControlFlags set to 0 to disable CPU rate control. ]*/
        cpu_rate_control_information.ControlFlags = 0;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_005: [ If percent_cpu is not JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL, internal_job_object_helper_set_cpu_limit shall set ControlFlags to JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, and CpuRate to percent_cpu times 100. ]*/
        cpu_rate_control_information.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        cpu_rate_control_information.CpuRate = percent_cpu * 100;
    }
    /*Codes_SRS_JOB_OBJECT_HELPER_88_006: [ internal_job_object_helper_set_cpu_limit shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and the JOBOBJECT_CPU_RATE_CONTROL_INFORMATION. ]*/
    if (!SetInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_rate_control_information, sizeof(cpu_rate_control_information)))
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_007: [ If SetInformationJobObject fails, internal_job_object_helper_set_cpu_limit shall fail and return a non-zero value. ]*/
        LogLastError("failure in SetInformationJobObject(job_object=%p, JobObjectCpuRateControlInformation, &cpu_rate_control_information=%p, sizeof(cpu_rate_control_information)=%zu)",
            job_object, &cpu_rate_control_information, sizeof(cpu_rate_control_information));
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_008: [ internal_job_object_helper_set_cpu_limit shall succeed and return 0. ]*/
        result = 0;
    }

    return result;
}


static int internal_job_object_helper_set_memory_limit(HANDLE job_object, uint32_t percent_physical_memory)
{
    int result;

    if (percent_physical_memory == JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_010: [ If percent_physical_memory is JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT, internal_job_object_helper_set_memory_limit shall call SetInformationJobObject passing JobObjectExtendedLimitInformation with LimitFlags set to 0 to remove memory limits. ]*/
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information = { 0 };
        if (!SetInformationJobObject(job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information)))
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_013: [ If there are any failures, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
            LogLastError("failure in SetInformationJobObject to remove memory limits (job_object=%p)", job_object);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_014: [ internal_job_object_helper_set_memory_limit shall succeed and return 0. ]*/
            result = 0;
        }
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_011: [ If percent_physical_memory is not JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT, internal_job_object_helper_set_memory_limit shall call GlobalMemoryStatusEx to get the total amount of physical memory. ]*/
        MEMORYSTATUSEX memory_status_ex;
        memory_status_ex.dwLength = sizeof(memory_status_ex);
        if (!GlobalMemoryStatusEx(&memory_status_ex))
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_013: [ If there are any failures, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
            LogLastError("failure in GlobalMemoryStatusEx(&memory_status_ex=%p)", &memory_status_ex);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_012: [ If percent_physical_memory is not JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT, internal_job_object_helper_set_memory_limit shall set JobMemoryLimit and ProcessMemoryLimit to percent_physical_memory percent of the physical memory and call SetInformationJobObject with JobObjectExtendedLimitInformation. ]*/
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information = { 0 };
            extended_limit_information.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_JOB_MEMORY | JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            SIZE_T memory_limit = percent_physical_memory * memory_status_ex.ullTotalPhys / 100;
            extended_limit_information.JobMemoryLimit = memory_limit;
            extended_limit_information.ProcessMemoryLimit = memory_limit;
            if (!SetInformationJobObject(job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information)))
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_013: [ If there are any failures, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
                LogLastError("failure in SetInformationJobObject(job_object=%p, JobObjectExtendedLimitInformation, &extended_limit_information=%p, sizeof(extended_limit_information)=%zu)",
                    job_object, &extended_limit_information, sizeof(extended_limit_information));
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_014: [ internal_job_object_helper_set_memory_limit shall succeed and return 0. ]*/
                result = 0;
            }
        }
    }

    return result;
}


/* Note: CPU and memory limits are always applied unconditionally (even if the values
   haven't changed) to keep the code simple. The caller can simply retry on failure. */
static int internal_job_object_helper_reconfigure(uint32_t percent_cpu, uint32_t percent_physical_memory)
{
    int result;

    /*Codes_SRS_JOB_OBJECT_HELPER_88_003: [ internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
    if (internal_job_object_helper_set_cpu_limit(job_object_singleton_state.job_object_helper->job_object, percent_cpu) != 0)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_017: [ If there are any failures, internal_job_object_helper_reconfigure shall fail and return a non-zero value. ]*/
        LogError("failure in internal_job_object_helper_set_cpu_limit(job_object=%p, percent_cpu=%" PRIu32 ") during reconfiguration",
            job_object_singleton_state.job_object_helper->job_object, percent_cpu);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_009: [ internal_job_object_helper_reconfigure shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
        if (internal_job_object_helper_set_memory_limit(job_object_singleton_state.job_object_helper->job_object, percent_physical_memory) != 0)
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_017: [ If there are any failures, internal_job_object_helper_reconfigure shall fail and return a non-zero value. ]*/
            LogError("failure in internal_job_object_helper_set_memory_limit(job_object=%p, percent_physical_memory=%" PRIu32 ") during reconfiguration",
                job_object_singleton_state.job_object_helper->job_object, percent_physical_memory);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_020: [ On successful reconfiguration, internal_job_object_helper_reconfigure shall return 0. ]*/
            result = 0;
        }
    }

    return result;
}


static int internal_job_object_helper_create(const char* job_name, uint32_t percent_cpu, uint32_t percent_physical_memory)
{
    int result;

    /*Codes_SRS_JOB_OBJECT_HELPER_88_035: [ internal_job_object_helper_create shall allocate a JOB_OBJECT_HELPER object. ]*/
    JOB_OBJECT_HELPER* job_object_helper = THANDLE_MALLOC(JOB_OBJECT_HELPER)(job_object_helper_dispose);
    if (job_object_helper == NULL)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_032: [ If there are any failures, internal_job_object_helper_create shall fail and return a non-zero value. ]*/
        LogError("failure in THANDLE_MALLOC(JOB_OBJECT_HELPER)(job_object_helper_dispose=%p)", job_object_helper_dispose);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_036: [ internal_job_object_helper_create shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes. ]*/
        job_object_helper->job_object = CreateJobObjectA(NULL, job_name);
        if (job_object_helper->job_object == NULL)
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_032: [ If there are any failures, internal_job_object_helper_create shall fail and return a non-zero value. ]*/
            LogLastError("failure in CreateJobObjectA(lpJobAttributes=NULL, job_name=%s)", MU_P_OR_NULL(job_name));
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_037: [ internal_job_object_helper_create shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the Windows job object. ]*/
            if (internal_job_object_helper_set_cpu_limit(job_object_helper->job_object, percent_cpu) != 0)
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_032: [ If there are any failures, internal_job_object_helper_create shall fail and return a non-zero value. ]*/
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_038: [ internal_job_object_helper_create shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the Windows job object. ]*/
                if (internal_job_object_helper_set_memory_limit(job_object_helper->job_object, percent_physical_memory) != 0)
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_88_032: [ If there are any failures, internal_job_object_helper_create shall fail and return a non-zero value. ]*/
                }
                else
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_88_039: [ internal_job_object_helper_create shall call GetCurrentProcess to get the current process handle. ]*/
                    HANDLE current_process = GetCurrentProcess();
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_008: [ internal_job_object_helper_create shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
                    if (!AssignProcessToJobObject(job_object_helper->job_object, current_process))
                    {
                        /*Codes_SRS_JOB_OBJECT_HELPER_88_032: [ If there are any failures, internal_job_object_helper_create shall fail and return a non-zero value. ]*/
                        LogLastError("failure in AssignProcessToJobObject(job_object=%p, current_process=%p)", job_object_helper->job_object, current_process);
                    }
                    else
                    {
                        /*Codes_SRS_JOB_OBJECT_HELPER_88_024: [ On success, internal_job_object_helper_create shall store the THANDLE(JOB_OBJECT_HELPER) in the process-level singleton state. ]*/
                        THANDLE_INITIALIZE_MOVE(JOB_OBJECT_HELPER)(&job_object_singleton_state.job_object_helper, &job_object_helper);
                        /*Codes_SRS_JOB_OBJECT_HELPER_88_033: [ internal_job_object_helper_create shall succeed and return 0. ]*/
                        result = 0;
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
        result = MU_FAILURE;
    }

all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory)
{
    THANDLE(JOB_OBJECT_HELPER) result = NULL;

    if (
        /*Codes_SRS_JOB_OBJECT_HELPER_19_001: [ If percent_cpu is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
        percent_cpu > MAX_CPU_PERCENT ||
        /*Codes_SRS_JOB_OBJECT_HELPER_88_034: [ If percent_physical_memory is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
        percent_physical_memory > MAX_MEMORY_PERCENT)
    {
        LogError("Invalid arguments: job_name=%s, percent_cpu=%" PRIu32 ", percent_physical_memory=%" PRIu32 "", MU_P_OR_NULL(job_name), percent_cpu, percent_physical_memory);
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_030: [ If job_object_singleton_state.job_object_helper is not NULL, job_object_helper_set_job_limits_to_current_process shall not create a new job object. ]*/
        if (job_object_singleton_state.job_object_helper != NULL)
        {
            LogWarning("Reconfiguring existing process-level singleton Job Object (cpu: %" PRIu32 ", memory: %" PRIu32 ")",
                percent_cpu, percent_physical_memory);
            /*Codes_SRS_JOB_OBJECT_HELPER_88_002: [ If job_object_singleton_state.job_object_helper is not NULL, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_reconfigure to apply the limits to the existing job object. ]*/
            if (internal_job_object_helper_reconfigure(percent_cpu, percent_physical_memory) != 0)
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
                /* Error already logged */
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_021: [ If internal_job_object_helper_reconfigure returns 0, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
                THANDLE_INITIALIZE(JOB_OBJECT_HELPER)(&result, job_object_singleton_state.job_object_helper);
            }
        }
        else
        {
            if (
                /*Codes_SRS_JOB_OBJECT_HELPER_88_022: [ If job_object_singleton_state.job_object_helper is NULL and percent_cpu is JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL and percent_physical_memory is JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object. ]*/
                (percent_cpu == JOB_OBJECT_HELPER_DISABLE_CPU_RATE_CONTROL && percent_physical_memory == JOB_OBJECT_HELPER_DISABLE_MEMORY_LIMIT) ||
                /*Codes_SRS_JOB_OBJECT_HELPER_88_023: [ If job_object_singleton_state.job_object_helper is NULL and both percent_cpu and percent_physical_memory are 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object. ]*/
                (percent_cpu == MAX_CPU_PERCENT && percent_physical_memory == MAX_MEMORY_PERCENT))
            {
                LogWarning("No job object needed (percent_cpu=%" PRIu32 ", percent_physical_memory=%" PRIu32 " are effectively no limits)", percent_cpu, percent_physical_memory);
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_031: [ job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_create to create a new job object and assign it to the current process. ]*/
                if (internal_job_object_helper_create(job_name, percent_cpu, percent_physical_memory) != 0)
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return NULL. ]*/
                    /* Error already logged */
                }
                else
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_helper_set_job_limits_to_current_process shall succeed and return a JOB_OBJECT_HELPER object. ]*/
                    THANDLE_INITIALIZE(JOB_OBJECT_HELPER)(&result, job_object_singleton_state.job_object_helper);
                }
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test)
{
    LogWarning("job_object_helper_deinit_for_test called - this should only be used for test cleanup");
    /*Codes_SRS_JOB_OBJECT_HELPER_88_027: [ job_object_helper_deinit_for_test shall release the singleton THANDLE(JOB_OBJECT_HELPER) by assigning it to NULL. ]*/
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_singleton_state.job_object_helper, NULL);
}
