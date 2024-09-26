// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "stdint.h"

#include "macro_utils/macro_utils.h"
#include "c_logging/logger.h"

#include "c_pal/job_object_helper.h"


IMPLEMENT_MOCKABLE_FUNCTION(, int, job_object_helper_limit_resources, uint32_t, percent_physical_memory, uint32_t, percent_cpu)
{
    int result;
    LogInfo("job_object_helper_limit_resources called with parameters:"
        " uint32_t percent_physical_memory = %d"
        ", uint32_t percent_cpu = %d",
        percent_physical_memory,
        percent_cpu);

    /*Codes_SRS_JOB_OBJECT_HELPER_18_001: [ If percent_physical_memory is 0, job_object_helper_limit_resources shall fail and return a non-zero value. ] */
    /*Codes_SRS_JOB_OBJECT_HELPER_18_002: [ If percent_cpu is 0, job_object_helper_limit_resources shall fail and return a non-zero value. ] */
    /*Codes_SRS_JOB_OBJECT_HELPER_18_004: [ If percent_physical_memory is greater than 100, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
    /*Codes_SRS_JOB_OBJECT_HELPER_18_005: [ If percent_cpu is greater than 100, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
    if ((percent_physical_memory == 0) ||
        (percent_physical_memory > 100) || 
        (percent_cpu == 0) ||
        (percent_cpu > 100))
    {
        LogError("Invalid arguments:"
            " uint32_t percent_physical_memory = %d"
            ", uint32_t percent_cpu = %d",
            percent_physical_memory,
            percent_cpu);
        /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
        result = MU_FAILURE;
    }
    else
    {
        /*Codes_SRS_JOB_OBJECT_HELPER_18_006: [ job_object_helper_limit_resources shall call CreateJobObject to create a new job object passing NULL for both lpJobAttributes and lpName. ]*/
        HANDLE job_object = CreateJobObject(NULL, NULL);
        if (job_object == NULL)
        {
            LogLastError("CreateJobObject(NULL, NULL) failed");
            /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
            result = MU_FAILURE;
        }
        else
        {
            /*Codes_SRS_JOB_OBJECT_HELPER_18_007: [ job_object_helper_limit_resources shall call GetCurrentProcess to get the current process handle. ]*/
            HANDLE current_process = GetCurrentProcess();
            if (current_process == NULL)
            {
                LogLastError("GetCurrentProcess() failed");
                /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
                result = MU_FAILURE;
            }
            else
            {
                /*Codes_SRS_JOB_OBJECT_HELPER_18_008: [ job_object_helper_limit_resources shall call AssignProcessToJobObject to assign the current process to the new job object. ]*/
                if (AssignProcessToJobObject(job_object, current_process) == FALSE)
                {
                    LogLastError("AssignProcessToJobObject(HANDLE job_object = %p, HANDLE current_process = %p) failed", job_object, current_process); 
                    /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
                    result = MU_FAILURE;
                }
                else
                {
                    MEMORYSTATUSEX memory_status_ex;
                    memory_status_ex.dwLength = sizeof(memory_status_ex);
                    /*Codes_SRS_JOB_OBJECT_HELPER_18_009: [ job_object_helper_limit_resources shall call GlobalMemoryStatusEx to get the total amount of physical memory. ]*/
                    if (!GlobalMemoryStatusEx(&memory_status_ex))
                    {
                        LogLastError("GlobalMemoryStatus(&memory_status_ex = %p) failed", &memory_status_ex);
                        /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
                        result = MU_FAILURE;
                    }
                    else
                    {
                        /*Codes_SRS_JOB_OBJECT_HELPER_18_010: [ job_object_helper_limit_resources shall call SetInformationJobObject, passing JobObjectExtendedLimitInformation and a JOBOBJECT_EXTENDED_LIMIT_INFORMATION object with JOB_OBJECT_LIMIT_JOB_MEMORY set and JobMemoryLimit set to the percent_physical_memory percent of the physical memory in bytes. ]*/
                        JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended_limit_information = {0};
                        extended_limit_information.BasicLimitInformation.LimitFlags  = JOB_OBJECT_LIMIT_JOB_MEMORY;
                        extended_limit_information.JobMemoryLimit = percent_physical_memory * memory_status_ex.ullTotalPhys / 100;
                        if (!SetInformationJobObject(job_object, JobObjectExtendedLimitInformation, &extended_limit_information, sizeof(extended_limit_information)))
                        {
                            LogLastError("SetInformationJobObject("
                                " job_object = %p"
                                ", JobObjectExtendedLimitInformation"
                                ", &extended_limit_information = %p"
                                ", sizeof(extended_limit_information) = %zu) failed",
                                job_object,
                                &extended_limit_information,
                                sizeof(extended_limit_information));
                            /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
                            result = MU_FAILURE;
                        }
                        else
                        {
                            /*Codes_SRS_JOB_OBJECT_HELPER_18_011: [ job_object_helper_limit_resources shall call SetInformationJobObject passing JobObjectCpuRateControlInformation and a JOBOBJECT_CPU_RATE_CONTROL_INFORMATION object with JOB_OBJECT_CPU_RATE_CONTROL_ENABLE and JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP set, and CpuRate set to percent_cpu times 100. ]*/
                            JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu_rate_control_information = {0};
                            cpu_rate_control_information.ControlFlags = JOB_OBJECT_CPU_RATE_CONTROL_ENABLE | JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
                            cpu_rate_control_information.CpuRate = percent_cpu * 100;
                            if (!SetInformationJobObject(job_object, JobObjectCpuRateControlInformation, &cpu_rate_control_information, sizeof(cpu_rate_control_information)))
                            {
                                LogLastError("SetInformationJobObject("
                                    " job_object = %p"
                                    ", JobObjectCpuRateControlInformation"
                                    ", &cpu_rate_control_information = %p"
                                    ", sizeof(cpu_rate_control_information) = %zu) failed",
                                    job_object,
                                    &cpu_rate_control_information,
                                    sizeof(cpu_rate_control_information));
                                /*Codes_SRS_JOB_OBJECT_HELPER_18_015: [ If there are any failures, job_object_helper_limit_resources shall fail and return a non-zero value. ]*/
                                result = MU_FAILURE;
                            }
                            else
                            {
                                /*Codes_SRS_JOB_OBJECT_HELPER_18_014: [ job_object_helper_limit_resources shall succeed and return 0. ]*/
                                result = 0;
                            }
                        }
                    }
                }
                /*Codes_SRS_JOB_OBJECT_HELPER_18_012: [ job_object_helper_limit_resources shall call CloseHandle to close the handle of the current process. ]*/
                if (!CloseHandle(current_process))
                {
                    LogLastError("CloseHandle(current_process = %p) failed", current_process);
                }
            }
            /*Codes_SRS_JOB_OBJECT_HELPER_18_013: [ job_object_helper_limit_resources shall call CloseHandle to close the handle to the job object. ]*/
            if (!CloseHandle(job_object))
            {
                LogLastError("CloseHandle(job_object = %p) failed", job_object);
            }
        }
    }

    LogInfo("job_object_helper_limit_resources exited with result = %d", result);
    return result;
}