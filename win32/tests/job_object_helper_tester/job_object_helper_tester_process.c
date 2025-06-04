/*Copyright(C) Microsoft Corporation.All rights reserved.*/

#include <stdio.h>
#include <stdlib.h>

#include "windows.h"

#include "c_pal/job_object_helper.h"

#define SLEEP_INTERVAL_MILLIS 30000
#define MEMORY_ALLOCATION_FAILED -10

int main(int argc, char* argv[])
{
    int result;

    if (argc < 4)
    {
        (void)printf("Usage: job_object_helper_tester.exe <JobObjectName> <MaxCpuPercentage> <MaxPhysicalMemoryPercentage> [AllocateMemorySize]\n");
        result = 1;
    }
    else
    {
        const char* job_name = argv[1];
        int max_cpu_percentage = atoi(argv[2]);
        int max_physical_memory_percentage = atoi(argv[3]);
        int allocate_memory_size = 0;
        if (argc >= 5)
        {
            allocate_memory_size = atoi(argv[4]);
        }

        (void)printf("JobName: %s\n", job_name);
        (void)printf("MaxCpuPercentage: %d\n", max_cpu_percentage);
        (void)printf("MaxPhysicalMemoryPercentage: %d\n", max_physical_memory_percentage);
        (void)printf("AllocateMemorySize: %d Bytes\n", allocate_memory_size);

        /* Call job_object_helper_set_job_limits_to_current_process for seting job limits */
        THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(
            job_name,
            max_cpu_percentage,
            max_physical_memory_percentage);

        if (job_object_helper == NULL)
        {
            (void)printf("Failed to set job limits\n");
            result = 1;
        }
        else
        {
            result = 0;
            void* p = NULL;
            if (allocate_memory_size != 0)
            {
                (void)printf("Allocate memory size: %d Bytes\n", allocate_memory_size);
                p = malloc(allocate_memory_size);
                if (!p)
                {
                    (void)printf("Memory allocation failed\n");
                    result = MEMORY_ALLOCATION_FAILED;
                }
                else
                {
                    Sleep(10000); /* hold the memory for few seconds so that other process will hit the limit */
                    free(p);
                    result = 0;
                }
            }
            /* Sleep for 30 sec. allow parent to perform few things before this exits */
            Sleep(SLEEP_INTERVAL_MILLIS);
            THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
        }
    }
    return result;
}
