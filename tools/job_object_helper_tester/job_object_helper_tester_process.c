/*Copyright(C) Microsoft Corporation.All rights reserved.*/

#include "c_pal/job_object_helper.h"
#include <stdlib.h>
#include <stdint.h>
#include "windows.h"

int main(int argc, char* argv[])
{

    if (argc != 4) {
        printf("Usage: target.exe <JobObjectName> <MaxCpuPercentage> <MaxPhysicalMemoryPercentage>\n");
        return 1;
    }

    const char* job_name = argv[1];
    int max_cpu_percentage = atoi(argv[2]);
    int max_physical_memory_percentage = atoi(argv[3]);

    printf("Received JobName: %s\n", job_name);
    printf("Received MaxCpuPercentage: %d\n", max_cpu_percentage);
    printf("Received MaxPhysicalMemoryPercentage: %d\n", max_physical_memory_percentage);

    // Call job_object_helper_set_job_limits_to_current_process
    // and wait for few seconds
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(
        job_name,
        max_cpu_percentage,
        max_physical_memory_percentage);

    if (job_object_helper == NULL)
    {
        printf("Failed to set job limits\n");
        return -1;
    }

    // Sleep for 1 mins
    Sleep(1 * 60 * 1000);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
    return 0;
}
