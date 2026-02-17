// Copyright(C) Microsoft Corporation.All rights reserved.


#include <stdlib.h>
#include <inttypes.h>

#include "windows.h"

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/job_object_helper.h"
#include "c_pal/timer.h"
#include "c_pal/uuid.h"


#define MEGABYTE ((size_t)1024 * 1024)
#define GIGABYTE (MEGABYTE * 1024)
#define MAX_MEMORY_POSSIBLE (GIGABYTE * 256)
#define TEST_BUFFER_SIZE (MEGABYTE * 512)
#define MAX_BUFFERS_BEFORE_FAILURE (MAX_MEMORY_POSSIBLE / TEST_BUFFER_SIZE)
#define NUM_TEST_PROCESSES 5
#define NUM_ALLOCATE_MEMORY_BLOCKS 12
#define NUM_RETRY 3
#define TEST_JOB_NAME_PREFIX "job_test_ebs_"
#define MAX_CPU_PERCENT 100
#define MAX_MEMORY_PERCENT 100
#define MAX_REPEATED_CALLS 10

static const size_t max_buffers_before_failure = MAX_BUFFERS_BEFORE_FAILURE;

/* CPU burn infrastructure for multi-threaded saturation tests.
   Single-threaded CPU burn is useless on multi-core machines — a single
   thread only uses 1/N of total capacity and never triggers the cap.
   We must saturate all cores to observe the JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP. */
static volatile LONG cpu_burn_active = 0;

static size_t get_malloc_limit_before_failure(void)
{
    /* malloc until failure and return the count of (512MB) buffers we were able to malloc. */
    void** buffers = malloc(sizeof(void*) * max_buffers_before_failure);
    ASSERT_IS_NOT_NULL(buffers);

    size_t max_buffers = 0;
    for (size_t i = 0; i < max_buffers_before_failure; i++)
    {
        buffers[i] = malloc(TEST_BUFFER_SIZE);
        if (buffers[i] == NULL)
        {
            max_buffers = i;
            break;
        }
    }
    ASSERT_ARE_NOT_EQUAL(size_t, 0, max_buffers);
    for (size_t i = 0; i < max_buffers; i++)
    {
        free(buffers[i]);
    }
    free(buffers);

    return max_buffers;
}


/* We need something to keep the CPU busy. */
/* If you list the numbers from 0 to 2^28-1 in binary, you'll find that 3,758,096,384 bits are set. */
/* Lets verify this. It takes about 5 seconds on my desktop PC. */
static size_t limit = 1 << 28;
static size_t expected_bits = 3758096384;

static size_t get_elapsed_milliseconds_for_cpu_bound_task(void)
{
    double start = timer_global_get_elapsed_ms();

    size_t total_bits = 0;

    for (size_t i = 0; i < limit; i++)
    {
        size_t this = i;
        while (this != 0)
        {
            total_bits += 1;
            this = this & (this - 1);
        }
    }

    double end = timer_global_get_elapsed_ms();
    ASSERT_ARE_EQUAL(size_t, expected_bits, total_bits);
    return (size_t)(end - start);
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
    job_object_helper_deinit();
}


TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process)
{
    /* Check that calling job_object_helper_set_job_limits_to_current_process
    *  1. creates the job object
    *  2. assigns the current process to the job object
    *  3. sets the job limits
    */

    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with job name: %s...", job_name);

    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(result);

    /* Check that the job object was created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object, "Failed to open job object");

    /* Query the Job Object to check if it has 1 process associated with it */
    JOBOBJECT_BASIC_PROCESS_ID_LIST process_id_list;
    ZeroMemory(&process_id_list, sizeof(process_id_list));
    DWORD return_length = 0;
    BOOL result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_ARE_EQUAL(ULONG, process_id_list.NumberOfAssignedProcesses, 1, "Job object should have 1 process associated with it");

    /* Check that the current process is assigned to the job object */
    BOOL ret = FALSE;
    ASSERT_IS_TRUE(IsProcessInJob(GetCurrentProcess(), job_object, &ret));
    ASSERT_IS_TRUE(ret, "Current process should be assigned to the job object");

    /* get the 1% of the total physical memory and check if that is equal to job objects memory limit*/
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));
    SIZE_T total_memory = mem_status.ullTotalPhys;
    SIZE_T one_percent_of_total_memory_in_MB = total_memory / 100 / MEGABYTE;
    LogInfo("Total Memory: %zu", total_memory);
    LogInfo("1%% of Total Memory: %zu", one_percent_of_total_memory_in_MB);

    /* Check that the job limits were set */
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_info;
    ZeroMemory(&job_info, sizeof(job_info));
    return_length = 0;
    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    LogInfo("Job memory limit: %zu", job_info.JobMemoryLimit / MEGABYTE);
    ASSERT_ARE_EQUAL(size_t, job_info.JobMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have job memory limit set to 1%% of total physical memory");
    ASSERT_ARE_EQUAL(size_t, job_info.ProcessMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have process memory limit set to 1%% of total physical memory");

    /* Performing the same action from the same process shall not change anything */
    THANDLE(JOB_OBJECT_HELPER) result_1 = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(result_1);

    job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NOT_NULL(job_object);

    /* Even though API was called twice, however since it was called from the same process, it shall show only one process is associated with it.*/
    result_query = QueryInformationJobObject(job_object, JobObjectBasicProcessIdList, &process_id_list, sizeof(process_id_list), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    ASSERT_ARE_EQUAL(int, process_id_list.NumberOfAssignedProcesses, 1, "Job object should have 1 process associated with it");

    ZeroMemory(&job_info, sizeof(job_info));
    return_length = 0;
    result_query = QueryInformationJobObject(job_object, JobObjectExtendedLimitInformation, &job_info, sizeof(job_info), &return_length);
    ASSERT_IS_TRUE(result_query, "Failed to query job object");
    LogInfo("Job memory limit: %zu", job_info.JobMemoryLimit / MEGABYTE);
    ASSERT_ARE_EQUAL(size_t, job_info.JobMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have job memory limit set to 1%% of total physical memory");
    ASSERT_ARE_EQUAL(size_t, job_info.ProcessMemoryLimit / MEGABYTE, one_percent_of_total_memory_in_MB, "Job object should have process memory limit set to 1%% of total physical memory");

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result, NULL);
    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&result_1, NULL);
}

void get_job_object_helper_tester_excutable_path(char* full_path, size_t full_path_size)
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

TEST_FUNCTION(test_job_object_helper_set_job_limits_to_current_process_check_memory_limits)
{
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with Job name: %s...", job_name);

    /* Get the total available Memory */
    MEMORYSTATUSEX mem_status;
    mem_status.dwLength = sizeof(mem_status);
    ASSERT_IS_TRUE(GlobalMemoryStatusEx(&mem_status));

    SIZE_T total_memory = mem_status.ullTotalPhys;
    /* Calculate 1% of the memory */
    SIZE_T one_percent_of_total_memory = total_memory / 100;
    LogInfo("Total Memory: %zu", total_memory);
    LogInfo("1%% of Total Memory: %zu", one_percent_of_total_memory);

    /* Set the process's memory limit to 1% */
    THANDLE(JOB_OBJECT_HELPER) job_object_helper = job_object_helper_set_job_limits_to_current_process(job_name, 50, 1);
    ASSERT_IS_NOT_NULL(job_object_helper);

    /* allocations till 1% should pass */
    char* buffer[NUM_ALLOCATE_MEMORY_BLOCKS];
    uint64_t memory_size = one_percent_of_total_memory / 10;
    LogInfo("Allocating %" PRIu64 "in every allocation", memory_size);
    int allocation_failed = 0;
    for (int i = 0; i < NUM_ALLOCATE_MEMORY_BLOCKS; ++i)
    {
        buffer[i] = malloc(memory_size);
        if (buffer[i] == NULL) {
            allocation_failed++;
        }
    }
    /* After reaching the limit, allocations are expected to be failed */
    ASSERT_ARE_NOT_EQUAL(int, 0, allocation_failed);
    /* With the set limit, not all allocations should have failed*/
    ASSERT_ARE_NOT_EQUAL(int, NUM_ALLOCATE_MEMORY_BLOCKS, allocation_failed);

    /* Free the allocated memory */
    for (int i = 0; i < NUM_ALLOCATE_MEMORY_BLOCKS; ++i)
    {
        if (buffer[i])
            free(buffer[i]);
    }

    THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&job_object_helper, NULL);
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


static DWORD WINAPI cpu_burn_thread_func(LPVOID param)
{
    (void)param;
    while (InterlockedCompareExchange(&cpu_burn_active, 1, 1) == 1)
    {
        /* Busy spin to consume CPU */
        volatile int x = 0;
        for (int i = 0; i < 100000; i++)
        {
            x += i;
        }
    }
    return 0;
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_001: [ If percent_cpu is 100 and percent_physical_memory is 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object (100% CPU and 100% memory are effectively no limits). ]*/
TEST_FUNCTION(test_job_object_helper_100_percent_cpu_returns_null)
{
    /* 100% CPU with 100% memory is effectively no limit.
       The function should return NULL without creating a job object,
       avoiding any risk of compounding if called again. */

    ///act
    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process("", MAX_CPU_PERCENT, MAX_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(result, "100%% CPU with 100%% memory should return NULL (no job object needed)");
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_001: [ If percent_cpu is 100 and percent_physical_memory is 100, job_object_helper_set_job_limits_to_current_process shall return NULL without creating a job object (100% CPU and 100% memory are effectively no limits). ]*/
TEST_FUNCTION(test_job_object_helper_100_percent_cpu_and_memory_does_not_create_job_object)
{
    /* 100% CPU with 100% memory is effectively no limit.
       The function should return NULL and no job object should be created. */

    ///arrange
    UUID_T job_name_uuid;
    (void)uuid_produce(&job_name_uuid);

    char job_name[64];
    (void)snprintf(job_name, sizeof(job_name), TEST_JOB_NAME_PREFIX "%" PRI_UUID_T "", UUID_T_VALUES(job_name_uuid));
    LogInfo("Running test with job name: %s...", job_name);

    ///act
    THANDLE(JOB_OBJECT_HELPER) result = job_object_helper_set_job_limits_to_current_process(job_name, MAX_CPU_PERCENT, MAX_MEMORY_PERCENT);

    ///assert
    ASSERT_IS_NULL(result, "100%% CPU with 100%% memory should return NULL (no job object needed)");

    /* Verify that no job object was actually created */
    HANDLE job_object = OpenJobObjectA(JOB_OBJECT_QUERY, FALSE, job_name);
    ASSERT_IS_NULL(job_object, "No job object should have been created for 100%% CPU and 100%% memory");
}


/* Helper: saturate all CPU cores for burn_duration_ms milliseconds and return
   the measured CPU usage as a percentage of total available capacity. */
static double measure_cpu_percent_under_saturation(DWORD burn_duration_ms)
{
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    DWORD num_cpus = sys_info.dwNumberOfProcessors;

    InterlockedExchange(&cpu_burn_active, 1);

    HANDLE* threads = (HANDLE*)malloc(num_cpus * sizeof(HANDLE));
    ASSERT_IS_NOT_NULL(threads);

    for (DWORD i = 0; i < num_cpus; i++)
    {
        threads[i] = CreateThread(NULL, 0, cpu_burn_thread_func, NULL, 0, NULL);
        ASSERT_IS_NOT_NULL(threads[i]);
    }

    /* Record process CPU time at start */
    FILETIME creation, exit_time, kernel_start, user_start;
    BOOL times_ok = GetProcessTimes(GetCurrentProcess(), &creation, &exit_time, &kernel_start, &user_start);
    ASSERT_IS_TRUE(times_ok, "GetProcessTimes failed at start");

    double start_ms = timer_global_get_elapsed_ms();

    Sleep(burn_duration_ms);

    double end_ms = timer_global_get_elapsed_ms();

    /* Record process CPU time at end */
    FILETIME kernel_end, user_end;
    times_ok = GetProcessTimes(GetCurrentProcess(), &creation, &exit_time, &kernel_end, &user_end);
    ASSERT_IS_TRUE(times_ok, "GetProcessTimes failed at end");

    /* Stop burn threads */
    InterlockedExchange(&cpu_burn_active, 0);
    (void)WaitForMultipleObjects(num_cpus, threads, TRUE, 10000);
    for (DWORD i = 0; i < num_cpus; i++)
    {
        (void)CloseHandle(threads[i]);
    }
    free(threads);

    /* Calculate actual CPU usage as percentage of total available */
    ULARGE_INTEGER k_start, k_end, u_start, u_end;
    k_start.LowPart = kernel_start.dwLowDateTime;
    k_start.HighPart = kernel_start.dwHighDateTime;
    k_end.LowPart = kernel_end.dwLowDateTime;
    k_end.HighPart = kernel_end.dwHighDateTime;
    u_start.LowPart = user_start.dwLowDateTime;
    u_start.HighPart = user_start.dwHighDateTime;
    u_end.LowPart = user_end.dwLowDateTime;
    u_end.HighPart = user_end.dwHighDateTime;

    uint64_t cpu_time_100ns = (k_end.QuadPart - k_start.QuadPart) + (u_end.QuadPart - u_start.QuadPart);
    double time_ms = end_ms - start_ms;
    uint64_t time_100ns = (uint64_t)(time_ms * 10000.0);
    uint64_t total_possible_100ns = time_100ns * num_cpus;

    double cpu_percent = (double)cpu_time_100ns / (double)total_possible_100ns * 100.0;

    LogInfo("CPU burn results: cpu_time=%" PRIu64 " (100ns ticks), time=%.0f ms, num_cpus=%lu, cpu_percent=%.1f%%",
        cpu_time_100ns, time_ms, (unsigned long)num_cpus, cpu_percent);

    return cpu_percent;
}

/*Tests_SRS_JOB_OBJECT_HELPER_88_002: [ If the process-level singleton job object has already been created with the same percent_cpu and percent_physical_memory values, job_object_helper_set_job_limits_to_current_process shall increment the reference count on the existing THANDLE(JOB_OBJECT_HELPER) and return it. ]*/
/*Tests_SRS_JOB_OBJECT_HELPER_88_004: [ On success, job_object_helper_set_job_limits_to_current_process shall store the THANDLE(JOB_OBJECT_HELPER) and the percent_cpu and percent_physical_memory values in static variables for the singleton pattern. ]*/
TEST_FUNCTION(test_job_object_helper_repeated_calls_return_same_singleton_and_no_cpu_compounding)
{
    /* This test calls job_object_helper_set_job_limits_to_current_process
       2, 5, and 9 times and verifies after each checkpoint:
       (a) Every returned THANDLE points to the same underlying object.
       (b) The CPU cap is still ~50%, not compounded (50%^2=25%, 50%^5≈3%, 50%^9≈0.2%). */

    ///arrange
    THANDLE(JOB_OBJECT_HELPER) handles[MAX_REPEATED_CALLS] = { NULL };

    /* checkpoints: after 2, 5, and 9 calls */
    static const int checkpoints[] = { 2, 5, 9 };
    static const int num_checkpoints = sizeof(checkpoints) / sizeof(checkpoints[0]);
    int calls_so_far = 0;

    for (int cp = 0; cp < num_checkpoints; cp++)
    {
        int target = checkpoints[cp];

        /* Issue calls up to the current checkpoint */
        for (int i = calls_so_far; i < target; i++)
        {
            THANDLE(JOB_OBJECT_HELPER) temp = job_object_helper_set_job_limits_to_current_process("", 50, 1);
            ASSERT_IS_NOT_NULL(temp, "Call %d should succeed", i + 1);
            THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&handles[i], temp);
            THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&temp, NULL);
        }
        calls_so_far = target;

        /* Verify all returned handles point to the same underlying object */
        for (int i = 1; i < target; i++)
        {
            ASSERT_ARE_EQUAL(void_ptr, (const void*)handles[0], (const void*)handles[i],
                "After %d calls, call %d should return the same singleton", target, i + 1);
        }
        LogInfo("After %d calls: singleton verified (all handles are the same pointer)", target);

        ///act
        /* Saturate CPU and measure — cap should still be ~50%, not compounded */
        double cpu_percent = measure_cpu_percent_under_saturation(3000);

        ///assert
        /* CPU should be around 50% (the cap), NOT compounded down.
           If compounding occurred: 50%^2=25%, 50%^5≈3.1%, 50%^9≈0.2% — all far below 30%.
           Use 30% as lower bound and 70% as upper bound. */
        ASSERT_IS_TRUE(cpu_percent > 30.0,
            "CPU usage %.1f%% is too low after %d calls - possible compounding detected (expected ~50%%)", cpu_percent, target);
        ASSERT_IS_TRUE(cpu_percent < 70.0,
            "CPU usage %.1f%% is too high after %d calls - cap may not be enforced (expected ~50%%)", cpu_percent, target);

        LogInfo("PASS: CPU usage %.1f%% after %d calls is within expected range [30%%, 70%%] (no compounding)", cpu_percent, target);
    }

    ///cleanup
    for (int i = 0; i < MAX_REPEATED_CALLS; i++)
    {
        THANDLE_ASSIGN(JOB_OBJECT_HELPER)(&handles[i], NULL);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
