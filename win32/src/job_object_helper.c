// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdbool.h>
#include <stdint.h>

#include "windows.h"
#include "processthreadsapi.h"
#include "jobapi2.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/job_object_helper.h"

#define MAX_CPU_PERCENT 100
#define MAX_MEMORY_PERCENT 100

/* Process-level singleton state to prevent Job Object accumulation.
   Windows Job Objects compound CPU rate limits multiplicatively when
   a process is assigned to multiple jobs. CloseHandle does NOT
   disassociate the process from the job. Since this function calls
   AssignProcessToJobObject on the current process, it must ensure
   that only one job object is ever created per process lifetime. */

typedef struct JOB_OBJECT_SINGLETON_STATE_TAG
{
    HANDLE job_object;
    uint32_t percent_cpu;
    uint32_t percent_memory;
} JOB_OBJECT_SINGLETON_STATE;

static JOB_OBJECT_SINGLETON_STATE job_object_singleton_state = { NULL, 0, 0 };

static int internal_job_object_helper_set_cpu_limit(HANDLE job_object, uint32_t percent_cpu)
{
    int result;

    if(percent_cpu == 0 || percent_cpu > MAX_CPU_PERCENT)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_004: [ If percent_cpu is 0 or greater than 100, internal_job_object_helper_set_cpu_limit shall fail and return a non-zero value. ]*/
        LogError("Invalid argument: percent_cpu=%" PRIu32 "", percent_cpu);
        result = MU_FAILURE;
    }
    else
    {
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_rate_control_information = { 0 };

        /*Codes_SRS_JOB_OBJECT_HELPER_88_005: [ internal_job_object_helper_set_cpu_limit shall set ControlFlags to JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP, and CpuRate to percent_cpu times 100. ]*/
        cpu_rate_control_information.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        cpu_rate_control_information.CpuRate = percent_cpu * 100;

        /*Codes_SRS_JOB_OBJECT_HELPER_88_006: [ internal_job_object_helper_set_cpu_limit shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and the JOBOBJECT_CPU_RATE_CONTROL_INFORMATION. ]*/
        if (!SetInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_rate_control_information, sizeof(cpu_rate_control_information)))
        {
            LogLastError("failure in SetInformationJobObject(job_object=%p, JobObjectCpuRateControlInformation, &cpu_rate_control_information=%p, sizeof(cpu_rate_control_information)=%zu)",
                job_object, &cpu_rate_control_information, sizeof(cpu_rate_control_information));
            /*Codes_SRS_JOB_OBJECT_HELPER_88_007: [ If SetInformationJobObject fails, internal_job_object_helper_set_cpu_limit shall fail and return a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_042: [ On success, internal_job_object_helper_set_cpu_limit shall store percent_cpu in the singleton state. ]*/
            job_object_singleton_state.percent_cpu = percent_cpu;
            /*Codes_SRS_JOB_OBJECT_HELPER_88_008: [ internal_job_object_helper_set_cpu_limit shall succeed and return 0. ]*/
            result = 0;
        }
    }

    return result;
}


static int internal_job_object_helper_set_memory_limit(HANDLE job_object, uint32_t percent_physical_memory)
{
    int result;

    if(percent_physical_memory == 0 || percent_physical_memory > MAX_MEMORY_PERCENT)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_010: [ If percent_physical_memory is 0 or greater than 100, internal_job_object_helper_set_memory_limit shall fail and return a non-zero value. ]*/
        LogError("Invalid argument: percent_physical_memory=%" PRIu32 "", percent_physical_memory);
        result = MU_FAILURE;
    }
    else
    {

        /*Codes_SRS_JOB_OBJECT_HELPER_88_011: [ internal_job_object_helper_set_memory_limit shall call GlobalMemoryStatusEx to get the total amount of physical memory. ]*/
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
            /*Codes_SRS_JOB_OBJECT_HELPER_88_012: [ internal_job_object_helper_set_memory_limit shall set JobMemoryLimit and ProcessMemoryLimit to percent_physical_memory percent of the physical memory and call SetInformationJobObject with JobObjectExtendedLimitInformation. ]*/
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
                /*Codes_SRS_JOB_OBJECT_HELPER_88_043: [ On success, internal_job_object_helper_set_memory_limit shall store percent_physical_memory in the singleton state. ]*/
                job_object_singleton_state.percent_memory = percent_physical_memory;
                /*Codes_SRS_JOB_OBJECT_HELPER_88_014: [ internal_job_object_helper_set_memory_limit shall succeed and return 0. ]*/
                result = 0;
            }
        }
    }

    return result;
}


/* Note: CPU and memory limits are always applied unconditionally (even if the values
   haven't changed) to keep the code simple. The caller can simply retry on failure. */
static int internal_job_object_helper_update(uint32_t percent_cpu, uint32_t percent_physical_memory)
{
    int result;

    bool failed = false;
    /*Codes_SRS_JOB_OBJECT_HELPER_88_003: [ If job_object_singleton_state.job_object is not NULL and percent_cpu is not 0, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the existing job object. ]*/
    if(percent_cpu != 0)
    {
        if (internal_job_object_helper_set_cpu_limit(job_object_singleton_state.job_object, percent_cpu) != 0)
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
            LogError("failure in internal_job_object_helper_set_cpu_limit(job_object=%p, percent_cpu=%" PRIu32 ") during reconfiguration",
                job_object_singleton_state.job_object, percent_cpu);
            failed = true;
        }
        else
        {
            /* Do Nothing */
        }
    }
    else
    {
        /* Do Nothing */
    }

    if (failed)
    {
        /* Error already logged */
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_009: [ If job_object_singleton_state.job_object is not NULL and percent_physical_memory is not 0, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the existing job object. ]*/
        if(percent_physical_memory != 0)
        {
            if (internal_job_object_helper_set_memory_limit(job_object_singleton_state.job_object, percent_physical_memory) != 0)
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                LogError("failure in internal_job_object_helper_set_memory_limit(job_object=%p, percent_physical_memory=%" PRIu32 ") during reconfiguration",
                    job_object_singleton_state.job_object, percent_physical_memory);
                failed = true;
            }
            else
            {
                /* Do Nothing */
            }
        }
        else
        {
            /* Do Nothing */
        }
    }

    if (failed)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_047: [ On successful update of the existing job object, job_object_helper_set_job_limits_to_current_process shall return 0. ]*/
        result = 0;
    }

    return result;
}


static int internal_job_object_helper_create(const char* job_name, uint32_t percent_cpu, uint32_t percent_physical_memory)
{
    int result;

    /*Codes_SRS_JOB_OBJECT_HELPER_88_036: [ job_object_helper_set_job_limits_to_current_process shall call CreateJobObjectA passing job_name for lpName and NULL for lpJobAttributes. ]*/
    HANDLE job_object = CreateJobObjectA(NULL, job_name);
    if (job_object == NULL)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
        LogLastError("failure in CreateJobObjectA(lpJobAttributes=NULL, job_name=%s)", MU_P_OR_NULL(job_name));
        result = MU_FAILURE;
    }
    else
    {
        bool failed = false;
        /*Codes_SRS_JOB_OBJECT_HELPER_88_037: [ If percent_cpu is not 0, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_set_cpu_limit to apply the CPU rate control to the new job object. ]*/
        if(percent_cpu != 0)
        {
            if (internal_job_object_helper_set_cpu_limit(job_object, percent_cpu) != 0)
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                LogError("failure in internal_job_object_helper_set_cpu_limit(job_object=%p, percent_cpu=%" PRIu32 ") during creation",
                    job_object, percent_cpu);
                failed = true;
            }
        }
        else
        {
            /* Do Nothing*/
        }

        if(failed)
        {
            /* Error already logged */
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_038: [ If percent_physical_memory is not 0, job_object_helper_set_job_limits_to_current_process shall call internal_job_object_helper_set_memory_limit to apply the memory limit to the new job object. ]*/
            if(percent_physical_memory != 0)
            {
                if (internal_job_object_helper_set_memory_limit(job_object, percent_physical_memory) != 0)
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                    LogError("failure in internal_job_object_helper_set_memory_limit(job_object=%p, percent_physical_memory=%" PRIu32 ") during creation",
                        job_object, percent_physical_memory);
                    failed = true;
                }
                else
                {
                    /* Do Nothing */
                }
            }
            else
            {
                /* Do Nothing */
            }
        }

        if (failed)
        {
            /* Error already logged */
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_88_039: [ job_object_helper_set_job_limits_to_current_process shall call GetCurrentProcess to get the current process handle. ]*/
            HANDLE current_process = GetCurrentProcess();
            /*Codes_SRS_JOB_OBJECT_HELPER_19_008: [ job_object_helper_set_job_limits_to_current_process shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
            if (!AssignProcessToJobObject(job_object, current_process))
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                LogLastError("failure in AssignProcessToJobObject(job_object=%p, current_process=%p)", job_object, current_process);
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_88_024: [ On success, job_object_helper_set_job_limits_to_current_process shall store the job object HANDLE in the process-level singleton state. ]*/
                job_object_singleton_state.job_object = job_object;
                /*Codes_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_helper_set_job_limits_to_current_process shall succeed and return 0. ]*/
                result = 0;
                goto all_ok;
            }
        }

        if (!CloseHandle(job_object))
        {
            LogLastError("failure in CloseHandle(job_object=%p)", job_object);
        }
        result = MU_FAILURE;
    }

all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_set_job_limits_to_current_process, const char*, job_name, uint32_t, percent_cpu, uint32_t, percent_physical_memory)
{
    int result;

    if (
        /*Codes_SRS_JOB_OBJECT_HELPER_19_001: [ If percent_cpu is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
        percent_cpu > MAX_CPU_PERCENT ||
        /*Codes_SRS_JOB_OBJECT_HELPER_88_034: [ If percent_physical_memory is greater than 100 then job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
        percent_physical_memory > MAX_MEMORY_PERCENT ||
        /*Codes_SRS_JOB_OBJECT_HELPER_88_040: [ If percent_cpu is 0 and percent_physical_memory is 0 then job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
        (percent_cpu == 0 && percent_physical_memory == 0))
    {
        LogError("Invalid arguments: job_name=%s, percent_cpu=%" PRIu32 ", percent_physical_memory=%" PRIu32 "", MU_P_OR_NULL(job_name), percent_cpu, percent_physical_memory);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_88_030: [ If job_object_singleton_state.job_object is not NULL, job_object_helper_set_job_limits_to_current_process shall not create a new job object. ]*/
        if (job_object_singleton_state.job_object != NULL)
        {
            LogWarning("Reconfiguring existing process-level singleton Job Object (cpu: %" PRIu32 ", memory: %" PRIu32 ")",
                percent_cpu, percent_physical_memory);

            if(
                /*Codes_SRS_JOB_OBJECT_HELPER_88_041: [ If job_object_singleton_state.job_object is not NULL and percent_cpu is 0 and the job_object_singleton_state.percent_cpu is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                (percent_cpu == 0 && job_object_singleton_state.percent_cpu != 0) ||
                /*Codes_SRS_JOB_OBJECT_HELPER_88_046: [ If job_object_singleton_state.job_object is not NULL and percent_physical_memory is 0 and the job_object_singleton_state.percent_memory is non-zero, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                (percent_physical_memory == 0 && job_object_singleton_state.percent_memory != 0))
            {
                LogError("Invalid arguments: percent_cpu or percent_physical_memory cannot be set to 0 once it has been set to a non-zero value. Received percent_cpu=%" PRIu32 ", received percent_physical_memory=%" PRIu32 ", current percent_cpu=%" PRIu32 ", current percent_physical_memory=%" PRIu32 "", percent_cpu, percent_physical_memory, job_object_singleton_state.percent_cpu, job_object_singleton_state.percent_memory);
                result = MU_FAILURE;
            }
            else
            {
                if (internal_job_object_helper_update(percent_cpu, percent_physical_memory) != 0)
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                    /* Error already logged */
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_88_047: [ On successful update of the existing job object, job_object_helper_set_job_limits_to_current_process shall return 0. ]*/
                    result = 0;
                }
            }
        }
        else
        {
            if (
                /*Codes_SRS_JOB_OBJECT_HELPER_88_023: [ If job_object_singleton_state.job_object is NULL and both percent_cpu and percent_physical_memory are 100, job_object_helper_set_job_limits_to_current_process shall return a non-zero value without creating a job object. ]*/
                (percent_cpu == MAX_CPU_PERCENT && percent_physical_memory == MAX_MEMORY_PERCENT))
            {
                LogWarning("No job object needed (percent_cpu=%" PRIu32 ", percent_physical_memory=%" PRIu32 " are effectively no limits)", percent_cpu, percent_physical_memory);
                result = MU_FAILURE;
            }
            else
            {
                if (internal_job_object_helper_create(job_name, percent_cpu, percent_physical_memory) != 0)
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_009: [ If there are any failures, job_object_helper_set_job_limits_to_current_process shall fail and return a non-zero value. ]*/
                    /* Error already logged */
                    result = MU_FAILURE;
                }
                else
                {
                    /*Codes_SRS_JOB_OBJECT_HELPER_19_010: [ job_object_helper_set_job_limits_to_current_process shall succeed and return 0. ]*/
                    result = 0;
                }
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, void, job_object_helper_deinit_for_test)
{
    LogWarning("job_object_helper_deinit_for_test called - this should only be used for test cleanup");

    /*Codes_SRS_JOB_OBJECT_HELPER_88_027: [ job_object_helper_deinit_for_test shall call CloseHandle to close the job object and set it to NULL. ]*/
    if (job_object_singleton_state.job_object != NULL)
    {
        if (!CloseHandle(job_object_singleton_state.job_object))
        {
            LogLastError("failure in CloseHandle(job_object_singleton_state.job_object=%p)", job_object_singleton_state.job_object);
        }
        job_object_singleton_state.job_object = NULL;
    }

    /*Codes_SRS_JOB_OBJECT_HELPER_88_044: [ job_object_helper_deinit_for_test shall reset percent_cpu to 0 in the singleton state. ]*/
    job_object_singleton_state.percent_cpu = 0;
    /*Codes_SRS_JOB_OBJECT_HELPER_88_045: [ job_object_helper_deinit_for_test shall reset percent_memory to 0 in the singleton state. ]*/
    job_object_singleton_state.percent_memory = 0;
}

IMPLEMENT_MOCKABLE_FUNCTION(, HANDLE, job_object_helper_get_internal_job_object_handle_for_test)
{
    /*Codes_SRS_JOB_OBJECT_HELPER_88_048: [ job_object_helper_get_internal_job_object_handle_for_test shall return the job object HANDLE from the singleton state. ]*/
    return job_object_singleton_state.job_object;
}
