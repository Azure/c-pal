// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#define JOB_OBJECT_HELPER               real_JOB_OBJECT_HELPER
#define PROCESS_HANDLE                  real_PROCESS_HANDLE

#define job_object_helper_create           real_job_object_helper_create 
#define job_object_helper_create_with_name real_job_object_helper_create_with_name
#define job_object_helper_get              real_job_object_helper_get
#define job_object_helper_assign_process   real_job_object_helper_assign_process
#define job_object_helper_limit_memory     real_job_object_helper_limit_memory
#define job_object_helper_limit_cpu        real_job_object_helper_limit_cpu