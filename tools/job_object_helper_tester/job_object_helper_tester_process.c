/*Copyright(C) Microsoft Corporation.All rights reserved.*/

#include "c_pal/job_object_helper.h"
#include <stdlib.h>
#include <stdint.h>
#include "windows.h"

int main()
{
    // Call job_object_helper_set_job_limits_to_current_process
    // and wait for few seconds
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process("//node/test_job", 50, 50);
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