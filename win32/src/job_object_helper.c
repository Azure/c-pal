// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdint.h>

#include "windows.h"
#include "processthreadsapi.h"
#include "jobapi.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/thandle.h"

#include "c_pal/job_object_helper.h"

typedef struct JOB_OBJECT_HELPER_TAG {
    HANDLE job_object;
} JOB_OBJECT_HELPER;

THANDLE_TYPE_DEFINE(JOB_OBJECT_HELPER);

typedef struct PROCESS_HANDLE_TAG {
    HANDLE process_hndl;
} PROCESS_HANDLE;

THANDLE_TYPE_DEFINE(PROCESS_HANDLE);

static void job_object_helper_dispose(JOB_OBJECT_HELPER* job_object_helper)
{
    /*Codes_SRS_JOB_OBJECT_HELPER_18_033: [ job_object_helper_dispose shall call CloseHandle to close the handle to the job object. ]*/
    if (!CloseHandle(job_object_helper->job_object))
    {
        LogLastError("failure in CloseHandle(job_object_helper->job_object=%p)", job_object_helper->job_object);
    }
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create)
{
    JOB_OBJECT_HELPER* result;
    JOB_OBJECT_HELPER* job_object_helper = THANDLE_MALLOC(JOB_OBJECT_HELPER)(job_object_helper_dispose);

    /*Codes_SRS_JOB_OBJECT_HELPER_18_016: [ job_object_helper_create shall allocate a JOB_OBJECT_HELPER object. ]*/
    if (job_object_helper == NULL)
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_18_018: [ If there are any failures, job_object_helper_create shall fail and return NULL. ]*/
        LogError("failure in THANDLE_MALLOC(JOB_OBJECT_HELPER)(job_object_helper_dispose=%p)", job_object_helper_dispose);
        result = NULL;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_18_024: [ job_object_helper_create shall call CreateJobObject to create a new job object passing NULL for both lpJobAttributes and lpName. ]*/
        job_object_helper->job_object = CreateJobObject(NULL, NULL);
        if (job_object_helper->job_object == NULL)
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_018: [ If there are any failures, job_object_helper_create shall fail and return NULL. ]*/
            LogLastError("failure in CreateJobObject(NULL, NULL)");
            result = NULL;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_025: [ job_object_helper_create shall call GetCurrentProcess to get the current process handle. ]*/
            HANDLE current_process = GetCurrentProcess();

            /*Codes_SRS_JOB_OBJECT_HELPER_18_026: [ job_object_helper_create shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
            BOOL assign_process_to_job_object_result = AssignProcessToJobObject(job_object_helper->job_object, current_process);
            /*Codes_SRS_JOB_OBJECT_HELPER_18_027: [ job_object_helper_create shall call CloseHandle to close the handle of the current process. ]*/
            if (!CloseHandle(current_process))
            {
                LogLastError("failure in CloseHandle(current_process=%p)", current_process);
            }

            if (!assign_process_to_job_object_result)
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_18_018: [ If there are any failures, job_object_helper_create shall fail and return NULL. ]*/
                LogLastError("failure in AssignProcessToJobObject(job_object=%p, current_process=%p)", job_object_helper->job_object, current_process);
                result = NULL;
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_18_019: [ job_object_helper_create shall succeed and return the JOB_OBJECT_HELPER object. ]*/
                result = job_object_helper;
                goto all_ok;
            }

            if (!CloseHandle(job_object_helper->job_object))
            {
                LogLastError("failure in CloseHandle(job_object_helper->job_object=%p)", job_object_helper->job_object);
            }
        }
        THANDLE_FREE(JOB_OBJECT_HELPER)(job_object_helper);

    }
all_ok:
    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_limit_memory, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_physical_memory)
{
    int result;

    /*Codes_SRS_JOB_OBJECT_HELPER_18_035: [ If job_object_helper is NULL, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
    /*Codes_SRS_JOB_OBJECT_HELPER_18_036: [ If percent_physical_memory is 0, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
    /*Codes_SRS_JOB_OBJECT_HELPER_18_037: [ If percent_physical_memory is greater than 100, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
    if (job_object_helper == NULL || percent_physical_memory == 0 || percent_physical_memory > 100)
    {
        LogError("Invalid arguments: job_object_helper=%p, percent_physical_memory=%" PRIu32 "", job_object_helper, percent_physical_memory);
        result = MU_FAILURE;
    }
    else
    {
        MEMORYSTATUSEX memory_status_ex;
        memory_status_ex.dwLength = sizeof(memory_status_ex);
        /*Codes_SRS_JOB_OBJECT_HELPER_18_023: [ job_object_helper_limit_memory shall call GlobalMemoryStatusEx to get the total amount of physical memory in kb. ]*/
        if (!GlobalMemoryStatusEx(&memory_status_ex))
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_018: [ If there are any failures, job_object_helper_create shall fail and return NULL. ]*/
            LogLastError("failure in GlobalMemoryStatus(&memory_status_ex=%p)", &memory_status_ex);
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_039: [ job_object_helper_limit_memory shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information = {0};
            extended_limit_information.BasicLimitInformation.LimitFlags  = JOB_OBJECT_LIMIT_JOB_MEMORY;
            extended_limit_information.JobMemoryLimit = percent_physical_memory * memory_status_ex.ullTotalPhys / 100;
            if (!SetInformationJobObject(job_object_helper->job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information)))
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_18_041: [ If there are any failures, job_object_helper_limit_memory shall fail and return a non-zero value. ]*/
                LogLastError("failure in SetInformationJobObject(job_object_helper->job_object=%p, JobObjectExtendedLimitInformation, &extended_limit_information=%p, sizeof(extended_limit_information)=%zu)",
                    job_object_helper->job_object, &extended_limit_information, sizeof(extended_limit_information));
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_18_042: [ job_object_helper_limit_memory shall succeed and return 0. ]*/
                result = 0;
            }
        }
    }

    return result;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_create_with_name, const char*, job_name)
{
    (void)job_name;
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, THANDLE(JOB_OBJECT_HELPER), job_object_helper_get, const char*, job_name)
{
    (void)job_name;
    return NULL;
}

IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_assign_process, THANDLE(JOB_OBJECT_HELPER), job_object_helper, THANDLE(PROCESS_HANDLE), process_hndl)
{
    (void)job_object_helper;
    (void)process_hndl;
    return MU_FAILURE;
}


IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_limit_cpu, THANDLE(JOB_OBJECT_HELPER), job_object_helper, uint32_t, percent_cpu)
{
    int result;

    /*Codes_SRS_JOB_OBJECT_HELPER_18_043: [ If job_object_helper is NULL, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
    /*Codes_SRS_JOB_OBJECT_HELPER_18_044: [ If percent_cpu is 0, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
    /*Codes_SRS_JOB_OBJECT_HELPER_18_045: [ If percent_cpu is greater than 100, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
    if (job_object_helper == NULL || percent_cpu == 0 || percent_cpu > 100)
    {
        LogError("Invalid arguments: job_object_helper=%p, percent_cpu=%" PRIu32 "", job_object_helper, percent_cpu);
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_18_047: [ job_object_helper_limit_cpu shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
        JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_rate_control_information = {0};
        cpu_rate_control_information.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        cpu_rate_control_information.CpuRate = percent_cpu * 100;
        if (!SetInformationJobObject(job_object_helper->job_object, JobObjectCpuRateControlInformation, &cpu_rate_control_information, sizeof(cpu_rate_control_information)))
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_049: [ If there are any failures, job_object_helper_limit_cpu shall fail and return a non-zero value. ]*/
            LogLastError("failure in SetInformationJobObject(job_object_helper->job_object=%p, JobObjectCpuRateControlInformation, &cpu_rate_control_information=%p, sizeof(cpu_rate_control_information)=%zu)",
                job_object_helper->job_object, &cpu_rate_control_information, sizeof(cpu_rate_control_information));
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_050: [ job_object_helper_limit_cpu shall succeed and return 0. ]*/
            result = 0;
        }
    }

    return result;

}

