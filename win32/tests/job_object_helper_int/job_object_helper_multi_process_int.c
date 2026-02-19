// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/job_object_helper.h"
#include "c_pal/uuid.h"


#define MEGABYTE ((size_t)1024 * 1024)
#define NUM_TEST_PROCESSES 5
#define NUM_RETRY 3
#define TEST_JOB_NAME_PREFIX "job_test_ebs_"

static void get_job_object_helper_tester_excutable_path(char* full_path, size_t full_path_size)
{
    /* Use GetModuleFileNameA to get the path of the current executable */
    char path[MAX_PATH];
    DWORD length = GetModuleFileNameA(NULL, path, sizeof(path));
    if (length == 0 || length >= sizeof(path))
    {
        LogLastError("GetModuleFileNameA failed");
        return;
    }

    /* Copy the full path to directory and remove the executable name */
    char directory[MAX_PATH];
    strcpy(directory, path);
    char* last_slash = strrchr(directory, '\\');
    if (last_slash != NULL)
    {
        *last_slash = '\0'; /* Truncate at the last backslash */
    }
    else
    {
        /* No backslash found, path contains only the file name */
        directory[0] = '.'; /* Set directory to current directory */
    }
    /* Create the full path to the job_object_helper_tester executable */
    (void)snprintf(full_path, full_path_size, "%s\\..\\job_object_helper_tester\\job_object_helper_tester.exe", directory);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
}

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
    job_object_helper_deinit_for_test();
}

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_from_multiple_processes)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with job name: %s...", job_name);

    char full_path[MAX_PATH];
    get_job_object_helper_tester_excutable_path(full_path, sizeof(full_path));
    ASSERT_ARE_NOT_EQUAL(size_t, 0, strlen(full_path), "Failed to get the full path to job_object_helper_tester executable");

    /* start 3 new process using CreateNewProcess, those processes shall
    call job_object_helper_set_job_limits_to_current_process */
    STARTUPINFOA si[NUM_TEST_PROCESSES];
    PROCESS_INFORMATION pi[NUM_TEST_PROCESSES];

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    char cmd_line[512];
    (void)snprintf(cmd_line, sizeof(cmd_line), "\"%s\" %s %d %d", full_path, job_name, 50, 1);

    for (int i = 0; i < NUM_TEST_PROCESSES; ++i)
    {
        LogInfo("Starting process: %d", i);
        si[i].cb = sizeof(si[i]);

        /* Start the process; this process calls job_object_helper_set_job_limits_to_current_process */
        (void)CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si[i], &pi[i]);

        /* sleep for 2 seconds to allow the child process to run */
        Sleep(2000);
        /* try for 3 times before declaring a failure, as it may take time for a new process to get scheduled and create the job object  */
        HANDLE job_object = NULL;
        for (int j = 0; j < NUM_RETRY; ++j)
        {
            job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
            if (job_object != NULL)
            {
                break;
            }
            LogInfo("OpenJobObjectA failed, will retry %d", j + 1);
            /* sleep for 2 seconds */
            Sleep(2000);
        }

        ASSERT_IS_NOT_NULL(job_object);

        DWORD return_length;
        JOBOBJECT_BASIC_PROCESS_ID_LIST pid_list;
        ZeroMemory(&pid_list, sizeof(pid_list));
        /* Ignoring return value as this will return an error ERROR_MORE_DATA*/
        (void)QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &pid_list, sizeof(pid_list), &return_length);
        ASSERT_ARE_EQUAL(ULONG, i+1, pid_list.NumberOfAssignedProcesses);
    }

    for (int i = 0; i < NUM_TEST_PROCESSES; ++i)
    {
        /* Terminate the process; Terminate is forceful */
        if (!TerminateProcess(pi[i].hProcess, 0))
        {
            LogLastError("TerminateProcess failed for Process %d", i);
        }

        /* Just to make sure, process has terminated */
        (void)WaitForSingleObject(pi[i].hProcess, INFINITE);

        /* Close process and thread handles. */
        if (!CloseHandle(pi[i].hProcess))
        {
            LogLastError("CloseHandle failed for hProcess handle for process: %d", i);
        }
        if (!CloseHandle(pi[i].hThread))
        {
            LogLastError("CloseHandle failed for hThread handle for process: %d", i);
        }
    }
}

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_from_multiple_processes_memory_limit_validation)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with job name: %s...", job_name);

    char full_path[MAX_PATH];
    get_job_object_helper_tester_excutable_path(full_path, sizeof(full_path));
    ASSERT_ARE_NOT_EQUAL(size_t, 0, strlen(full_path), "Failed to get the full path to job_object_helper_tester executable");

    /* Get the total available Memory */
    MEMORYSTATUSEX mem_status;
    ZeroMemory(&mem_status, sizeof(mem_status));
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));

    SIZE_T total_memory = mem_status.ullTotalPhys;
    /* Calculate 1% of the memory */
    SIZE_T one_percent_of_total_memory = total_memory / 100;
    LogInfo("Total Memory: %zu", total_memory);
    LogInfo("1%% of Total Memory: %zu", one_percent_of_total_memory);

    /* start multiple processes which will allocate half of this 1% each
    due to the limit set, few processes shall fail allocation*/
    STARTUPINFOA si[NUM_TEST_PROCESSES];
    PROCESS_INFORMATION pi[NUM_TEST_PROCESSES];
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    char cmd_line[512];
    (void)snprintf(cmd_line, sizeof(cmd_line), "\"%s\" %s %d %d %zu", full_path, job_name, 50, 1, one_percent_of_total_memory / 2);

    for (int i = 0; i < NUM_TEST_PROCESSES; ++i)
    {
        LogInfo("Starting process: %d", i);
        si[i].cb = sizeof(si[i]);

        /* Start the process; this process calls job_object_helper_set_job_limits_to_current_process */
        (void)CreateProcessA(NULL, cmd_line, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si[i], &pi[i]);

        /* sleep for 2 seconds to allow the child process to run */
        Sleep(2000);
        /* try for 3 times before declaring a failure, as it may take time for a new process to get scheduled and create the job object */
        HANDLE job_object = NULL;
        for (int j = 0; j < NUM_RETRY; ++j)
        {
            job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
            if (job_object != NULL)
            {
                break;
            }
            LogInfo("OpenJobObjectA failed, will retry %d", j + 1);
            /* sleep for 1 seconds before retrying*/
            Sleep(1000);
        }

        ASSERT_IS_NOT_NULL(job_object);

        DWORD return_length;
        JOBOBJECT_BASIC_PROCESS_ID_LIST pid_list;
        ZeroMemory(&pid_list, sizeof(pid_list));
        /* Ignoring return as this will return an error ERROR_MORE_DATA*/
        (void)QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &pid_list, sizeof(pid_list), &return_length);
        ASSERT_ARE_EQUAL(ULONG, i+1, pid_list.NumberOfAssignedProcesses);
    }

    int allocation_failed_process_count = 0;
    for (int i = 0; i < NUM_TEST_PROCESSES; ++i)
    {
        (void)WaitForSingleObject(pi[i].hProcess, INFINITE);

        DWORD exit_code = 0;
        // Get the exit code of the started process.
        if (GetExitCodeProcess(pi[i].hProcess, &exit_code))
        {
            if (exit_code != 0)
            {
                LogInfo("Process exited with non-zero error code");
                allocation_failed_process_count++;
            }
        }
        else
        {
            LogLastError("Failed to get exit code");
        }

        // Close process and thread handles.
        if (!CloseHandle(pi[i].hProcess))
        {
            LogLastError("CloseHandle failed for hProcess handle for process: %d", i);
        }
        if (!CloseHandle(pi[i].hThread))
        {
            LogLastError("CloseHandle failed for hThread handle for process: %d", i);
        }
    }

    ASSERT_ARE_NOT_EQUAL(int, 0, allocation_failed_process_count, "Few processes should have failed to allocate memory");
    ASSERT_ARE_NOT_EQUAL(int, NUM_TEST_PROCESSES, allocation_failed_process_count, "Not all processes should have failed to allocate memory");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
