// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "windows.h"

#include "testrunnerswitcher.h"

#include "macro_utils/macro_utils.h"

#include "c_logging/logger.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/robust_semaphore.h"

TEST_DEFINE_ENUM_TYPE(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_RESULT_VALUES);

#define TEST_WAIT_TIMEOUT_MS 10000
#define TEST_NO_MORE_ACQUISITIONS_WAIT_MS 300
#define TEST_TESTER_PROCESS_TIMEOUT_MS 60000
#define TEST_UNIT_PROBE_INTERVAL_MS 20
#define MAX_TEST_USERS 8

/* a user of the robust semaphore, for example one of the processes or components that need a unit */
typedef struct TEST_USER_TAG
{
    ROBUST_SEMAPHORE_HANDLE robust_semaphore;
    volatile_atomic int32_t completed;  /* 1 once on_acquire_complete was called */
    volatile_atomic int32_t result;     /* ROBUST_SEMAPHORE_ACQUIRE_RESULT */
    volatile_atomic int32_t unit_index;
    volatile_atomic int32_t released;
    volatile_atomic int64_t completed_ms;
    volatile_atomic int32_t slow_callback_returned_at_completion;
    volatile_atomic int32_t worker_thread_id;   /* the thread that called on_acquire_complete */
    void* volatile_atomic worker_thread;        /* opened by on_acquire_complete_open_worker_thread, closed by the test or by method_cleanup */
} TEST_USER;

static TEST_USER test_users[MAX_TEST_USERS];
static volatile_atomic int32_t completed_count;
static volatile_atomic int32_t current_holders;
static volatile_atomic int32_t max_holders;
static volatile_atomic int32_t slow_callback_returned;
static volatile_atomic int32_t acquire_from_callback_result;
static volatile_atomic int32_t first_worker_thread_id;
static char test_semaphore_name[96];

/* the tester process started by a test, killed and closed by method_cleanup (also when the test fails) */
static HANDLE test_job;
static PROCESS_INFORMATION test_tester_process;
static HANDLE test_held_event;

static void on_acquire_complete(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    TEST_USER* test_user = context;

    if (result != ROBUST_SEMAPHORE_ACQUIRE_OK)
    {
        /* do nothing, no unit is held */
    }
    else
    {
        int32_t holders = interlocked_increment(&current_holders);
        int32_t current_max;
        while ((current_max = interlocked_add(&max_holders, 0)) < holders)
        {
            (void)interlocked_compare_exchange(&max_holders, holders, current_max);
        }
    }

    (void)interlocked_exchange(&test_user->unit_index, (int32_t)unit_index);
    (void)interlocked_exchange(&test_user->result, result);
    (void)interlocked_exchange(&test_user->worker_thread_id, (int32_t)GetCurrentThreadId());
    (void)interlocked_exchange_64(&test_user->completed_ms, (int64_t)timer_global_get_elapsed_ms());
    (void)interlocked_exchange(&test_user->slow_callback_returned_at_completion, interlocked_add(&slow_callback_returned, 0));
    (void)interlocked_exchange(&test_user->completed, 1);
    (void)interlocked_increment(&completed_count);
    wake_by_address_all(&completed_count);
}

static void create_test_user(TEST_USER* test_user, const char* name, uint32_t max_count, const char* security_descriptor)
{
    test_user->robust_semaphore = robust_semaphore_create(name, max_count, security_descriptor);
    ASSERT_IS_NOT_NULL(test_user->robust_semaphore);
    (void)interlocked_exchange(&test_user->completed, 0);
    (void)interlocked_exchange(&test_user->result, ROBUST_SEMAPHORE_ACQUIRE_ERROR);
    (void)interlocked_exchange(&test_user->unit_index, (int32_t)ROBUST_SEMAPHORE_UNIT_INDEX_NONE);
    (void)interlocked_exchange(&test_user->released, 0);
    (void)interlocked_exchange(&test_user->worker_thread_id, 0);
    (void)interlocked_exchange_pointer(&test_user->worker_thread, NULL);
}

static void create_test_users(uint32_t count, const char* name, uint32_t max_count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        create_test_user(&test_users[i], name, max_count, NULL);
    }
}

static void acquire_test_user_with_callback(TEST_USER* test_user, uint32_t timeout_ms, uint32_t max_hold_ms, ROBUST_SEMAPHORE_ON_ACQUIRE_COMPLETE on_acquire_complete_callback)
{
    (void)interlocked_exchange(&test_user->completed, 0);
    (void)interlocked_exchange(&test_user->released, 0);
    ASSERT_ARE_EQUAL(int, 0, robust_semaphore_acquire_async(test_user->robust_semaphore, timeout_ms, max_hold_ms, on_acquire_complete_callback, test_user));
}

static void acquire_test_user(TEST_USER* test_user, uint32_t timeout_ms, uint32_t max_hold_ms)
{
    acquire_test_user_with_callback(test_user, timeout_ms, max_hold_ms, on_acquire_complete);
}

/* the acquisition did not complete yet, it is waiting for a unit */
static void assert_test_user_waits(TEST_USER* test_user)
{
    Sleep(TEST_NO_MORE_ACQUISITIONS_WAIT_MS);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_user->completed, 0));
}

static bool test_user_holds_a_unit(TEST_USER* test_user)
{
    return
        (interlocked_add(&test_user->completed, 0) == 1) &&
        (interlocked_add(&test_user->result, 0) == ROBUST_SEMAPHORE_ACQUIRE_OK) &&
        (interlocked_add(&test_user->released, 0) == 0);
}

static void assert_holders_have_different_units(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        for (uint32_t j = i + 1; j < count; j++)
        {
            if (test_user_holds_a_unit(&test_users[i]) && test_user_holds_a_unit(&test_users[j]))
            {
                ASSERT_ARE_NOT_EQUAL(int32_t, interlocked_add(&test_users[i].unit_index, 0), interlocked_add(&test_users[j].unit_index, 0));
            }
            else
            {
                /* do nothing, at least one of them does not hold a unit */
            }
        }
    }
}

static void release_test_user(TEST_USER* test_user)
{
    if (
        (interlocked_add(&test_user->completed, 0) == 1) &&
        (interlocked_add(&test_user->result, 0) == ROBUST_SEMAPHORE_ACQUIRE_OK) &&
        (interlocked_exchange(&test_user->released, 1) == 0)
        )
    {
        (void)interlocked_decrement(&current_holders);
    }
    else
    {
        /* do nothing, the user does not hold a unit */
    }
    robust_semaphore_release(test_user->robust_semaphore);
}

static void wait_for_completed_count(int32_t expected_count)
{
    double start = timer_global_get_elapsed_ms();
    int32_t current;
    while ((current = interlocked_add(&completed_count, 0)) < expected_count)
    {
        ASSERT_IS_TRUE(timer_global_get_elapsed_ms() - start < TEST_WAIT_TIMEOUT_MS, "timed out waiting for %" PRId32 " completions, got %" PRId32 "", expected_count, current);
        (void)wait_on_address(&completed_count, current, 100);
    }
}

static ROBUST_SEMAPHORE_ACQUIRE_RESULT get_result(TEST_USER* test_user)
{
    return (ROBUST_SEMAPHORE_ACQUIRE_RESULT)interlocked_add(&test_user->result, 0);
}

static void close_test_user_worker_thread(TEST_USER* test_user)
{
    HANDLE worker_thread = interlocked_exchange_pointer(&test_user->worker_thread, NULL);
    if (worker_thread == NULL)
    {
        /* do nothing, no worker thread was opened */
    }
    else
    {
        (void)CloseHandle(worker_thread);
    }
}

/* robust_semaphore_destroy completes a pending acquisition and waits for the worker thread */
static void destroy_test_users(void)
{
    for (uint32_t i = 0; i < MAX_TEST_USERS; i++)
    {
        if (test_users[i].robust_semaphore == NULL)
        {
            /* do nothing, not created or already destroyed */
        }
        else
        {
            robust_semaphore_destroy(test_users[i].robust_semaphore);
            test_users[i].robust_semaphore = NULL;
        }
        close_test_user_worker_thread(&test_users[i]);
    }
}

/* looks at the mutexes from outside of the module: a unit is held if another thread owns its mutex */
static uint32_t get_held_unit_count(const char* name, uint32_t max_count)
{
    uint32_t held_count = 0;

    for (uint32_t i = 0; i < max_count; i++)
    {
        char mutex_name[128];
        ASSERT_IS_TRUE(snprintf(mutex_name, sizeof(mutex_name), "%s_%" PRIu32 "", name, i) > 0);

        HANDLE mutex = OpenMutexA(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, mutex_name);
        DWORD open_error = GetLastError();
        if (mutex == NULL)
        {
            /* nobody has the mutex open, so nobody holds it */
            ASSERT_ARE_EQUAL(uint32_t, ERROR_FILE_NOT_FOUND, (uint32_t)open_error, "OpenMutexA(%s) failed", mutex_name);
        }
        else
        {
            DWORD wait_result = WaitForSingleObject(mutex, 0);
            if (
                (wait_result == WAIT_OBJECT_0) ||
                (wait_result == WAIT_ABANDONED)
                )
            {
                ASSERT_IS_TRUE(ReleaseMutex(mutex));
            }
            else
            {
                ASSERT_ARE_EQUAL(uint32_t, WAIT_TIMEOUT, (uint32_t)wait_result, "WaitForSingleObject(%s) failed", mutex_name);
                held_count++;
            }
            (void)CloseHandle(mutex);
        }
    }

    return held_count;
}

/* the worker threads release the mutexes asynchronously, so wait a bit for the expected state */
static void wait_for_held_unit_count(const char* name, uint32_t max_count, uint32_t expected_held_count)
{
    double start = timer_global_get_elapsed_ms();
    uint32_t held_count;
    while ((held_count = get_held_unit_count(name, max_count)) != expected_held_count)
    {
        ASSERT_IS_TRUE(timer_global_get_elapsed_ms() - start < TEST_WAIT_TIMEOUT_MS, "%" PRIu32 " units of %s are held, expected %" PRIu32 "", held_count, name, expected_held_count);
        Sleep(TEST_UNIT_PROBE_INTERVAL_MS);
    }
}

/* the tester is built in its own folder next to the folder of the test exe (and of the test dll, which vstest loads in its own host process) */
static void get_tester_path(char* tester_path, DWORD tester_path_size)
{
    HMODULE test_module;
    ASSERT_IS_TRUE(GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&test_users, &test_module));
    DWORD length = GetModuleFileNameA(test_module, tester_path, tester_path_size);
    ASSERT_IS_TRUE((length > 0) && (length < tester_path_size));

    char* last_separator = strrchr(tester_path, '\\');
    ASSERT_IS_NOT_NULL(last_separator);
    last_separator[1] = '\0';
    ASSERT_ARE_EQUAL(int, 0, strcat_s(tester_path, tester_path_size, "..\\robust_semaphore_win32_tester\\robust_semaphore_win32_tester.exe"));
}

/* starts a process that holds a unit of the semaphore, in a job that kills it when the test ends */
static void start_tester_process(const char* name)
{
    char held_event_name[128];
    ASSERT_IS_TRUE(snprintf(held_event_name, sizeof(held_event_name), "%s_held", name) > 0);
    test_held_event = CreateEventA(NULL, TRUE, FALSE, held_event_name);
    ASSERT_IS_NOT_NULL(test_held_event);

    test_job = CreateJobObjectA(NULL, NULL);
    ASSERT_IS_NOT_NULL(test_job);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION job_limits = { 0 };
    job_limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    ASSERT_IS_TRUE(SetInformationJobObject(test_job, JobObjectExtendedLimitInformation, &job_limits, sizeof(job_limits)));

    char tester_path[MAX_PATH];
    get_tester_path(tester_path, MAX_PATH);
    char command_line[MAX_PATH + 256];
    ASSERT_IS_TRUE(snprintf(command_line, sizeof(command_line), "\"%s\" %s %s", tester_path, name, held_event_name) > 0);

    STARTUPINFOA startup_info = { 0 };
    startup_info.cb = sizeof(startup_info);
    BOOL created = CreateProcessA(tester_path, command_line, NULL, NULL, FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED, NULL, NULL, &startup_info, &test_tester_process);
    ASSERT_IS_TRUE(created, "CreateProcessA(%s) failed with %lu", tester_path, GetLastError());
    ASSERT_IS_TRUE(AssignProcessToJobObject(test_job, test_tester_process.hProcess));
    ASSERT_ARE_NOT_EQUAL(uint32_t, (uint32_t)-1, (uint32_t)ResumeThread(test_tester_process.hThread));
}

typedef struct UNIT_OWNER_THREAD_CONTEXT_TAG
{
    char mutex_name[128];
    HANDLE owned_event;
    HANDLE exit_event;
} UNIT_OWNER_THREAD_CONTEXT;

/* owns a mutex of the semaphore and exits without releasing it (no ASSERTs, this is not the test thread) */
static int unit_owner_thread(void* context)
{
    int result;
    UNIT_OWNER_THREAD_CONTEXT* thread_context = context;
    HANDLE mutex = CreateMutexA(NULL, FALSE, thread_context->mutex_name);
    if (mutex == NULL)
    {
        result = 1;
    }
    else
    {
        if (
            (WaitForSingleObject(mutex, INFINITE) != WAIT_OBJECT_0) ||
            !SetEvent(thread_context->owned_event) ||
            (WaitForSingleObject(thread_context->exit_event, INFINITE) != WAIT_OBJECT_0)
            )
        {
            result = 1;
        }
        else
        {
            result = 0;
        }

        /* the thread exits while owning the mutex, which abandons it */
        (void)CloseHandle(mutex);
    }

    return result;
}

static void start_unit_owner_thread(UNIT_OWNER_THREAD_CONTEXT* owner_context, THREAD_HANDLE* owner_thread, const char* name, uint32_t unit_index)
{
    ASSERT_IS_TRUE(snprintf(owner_context->mutex_name, sizeof(owner_context->mutex_name), "%s_%" PRIu32 "", name, unit_index) > 0);
    owner_context->owned_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(owner_context->owned_event);
    owner_context->exit_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(owner_context->exit_event);

    ASSERT_ARE_EQUAL(int, THREADAPI_OK, ThreadAPI_Create(owner_thread, unit_owner_thread, owner_context));
    ASSERT_ARE_EQUAL(uint32_t, WAIT_OBJECT_0, (uint32_t)WaitForSingleObject(owner_context->owned_event, TEST_WAIT_TIMEOUT_MS), "the owner thread did not get the unit");
}

static void end_unit_owner_thread(UNIT_OWNER_THREAD_CONTEXT* owner_context, THREAD_HANDLE owner_thread)
{
    ASSERT_IS_TRUE(SetEvent(owner_context->exit_event));
    int owner_result;
    ASSERT_ARE_EQUAL(int, THREADAPI_OK, ThreadAPI_Join(owner_thread, &owner_result));
    ASSERT_ARE_EQUAL(int, 0, owner_result);
    (void)CloseHandle(owner_context->exit_event);
    (void)CloseHandle(owner_context->owned_event);
}

static volatile_atomic int32_t destroyed_from_callback;

static void on_acquire_complete_destroy(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    TEST_USER* test_user = context;
    on_acquire_complete(context, result, unit_index);
    robust_semaphore_destroy(test_user->robust_semaphore);
    test_user->robust_semaphore = NULL;
    (void)interlocked_exchange(&destroyed_from_callback, 1);
    wake_by_address_all(&destroyed_from_callback);
}

#define TEST_SLOW_CALLBACK_MS 1000

static void on_acquire_complete_slow(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    on_acquire_complete(context, result, unit_index);
    Sleep(TEST_SLOW_CALLBACK_MS);
    (void)interlocked_exchange(&slow_callback_returned, 1);
}

/* keeps a handle to the worker thread, to check later that it ended */
static void on_acquire_complete_open_worker_thread(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    TEST_USER* test_user = context;
    HANDLE worker_thread = OpenThread(SYNCHRONIZE, FALSE, GetCurrentThreadId());
    if (worker_thread == NULL)
    {
        /* do nothing, the test finds no worker thread handle (no ASSERTs, this is not the test thread) */
    }
    else
    {
        (void)interlocked_exchange_pointer(&test_user->worker_thread, worker_thread);
    }
    on_acquire_complete(context, result, unit_index);
}

/* gives the unit back and starts a new acquisition, which the worker thread that runs this callback takes when it returns */
static void on_acquire_complete_release_and_acquire_again(void* context, ROBUST_SEMAPHORE_ACQUIRE_RESULT result, uint32_t unit_index)
{
    TEST_USER* test_user = context;
    (void)interlocked_exchange(&first_worker_thread_id, (int32_t)GetCurrentThreadId());
    on_acquire_complete(context, result, unit_index);
    release_test_user(test_user);
    (void)interlocked_exchange(&test_user->released, 0);
    (void)interlocked_exchange(&acquire_from_callback_result, robust_semaphore_acquire_async(test_user->robust_semaphore, ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT, on_acquire_complete, test_user));
}

static HANDLE get_test_user_worker_thread(TEST_USER* test_user)
{
    return interlocked_compare_exchange_pointer(&test_user->worker_thread, NULL, NULL);
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    (void)interlocked_exchange(&completed_count, 0);
    (void)interlocked_exchange(&current_holders, 0);
    (void)interlocked_exchange(&max_holders, 0);
    (void)interlocked_exchange(&destroyed_from_callback, 0);
    (void)interlocked_exchange(&slow_callback_returned, 0);
    (void)interlocked_exchange(&acquire_from_callback_result, MU_FAILURE);
    (void)interlocked_exchange(&first_worker_thread_id, 0);
    test_job = NULL;
    test_held_event = NULL;
    (void)memset(&test_tester_process, 0, sizeof(test_tester_process));

    /* unique per test, so that tests (and parallel test runs) do not share units */
    static volatile_atomic int32_t test_index;
    ASSERT_IS_TRUE(snprintf(test_semaphore_name, sizeof(test_semaphore_name), "Local\\robust_semaphore_win32_int_%lu_%" PRId32 "", GetCurrentProcessId(), interlocked_increment(&test_index)) > 0);
}

/* when a test fails it may leave a tester process and robust semaphore objects behind */
TEST_FUNCTION_CLEANUP(method_cleanup)
{
    if (test_tester_process.hProcess == NULL)
    {
        /* do nothing, the test did not start a tester process */
    }
    else
    {
        (void)TerminateProcess(test_tester_process.hProcess, 1);
        (void)WaitForSingleObject(test_tester_process.hProcess, TEST_WAIT_TIMEOUT_MS);
        (void)CloseHandle(test_tester_process.hThread);
        (void)CloseHandle(test_tester_process.hProcess);
        (void)memset(&test_tester_process, 0, sizeof(test_tester_process));
    }

    if (test_job == NULL)
    {
        /* do nothing, the test did not create a job */
    }
    else
    {
        (void)CloseHandle(test_job);
        test_job = NULL;
    }

    if (test_held_event == NULL)
    {
        /* do nothing, the test did not create the held event */
    }
    else
    {
        (void)CloseHandle(test_held_event);
        test_held_event = NULL;
    }

    destroy_test_users();
}

/* many users ask for a unit at the same time and only max_count of them get one at once */
TEST_FUNCTION(at_most_max_count_units_are_held_at_the_same_time)
{
    // arrange
    const uint32_t user_count = 6;
    const uint32_t max_count = 2;
    create_test_users(user_count, test_semaphore_name, max_count);

    // act
    for (uint32_t i = 0; i < user_count; i++)
    {
        acquire_test_user(&test_users[i], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    }

    // assert
    uint32_t released_count = 0;
    while (released_count < user_count)
    {
        int32_t expected_acquired = (int32_t)(((max_count + released_count) < user_count) ? (max_count + released_count) : user_count);
        wait_for_completed_count(expected_acquired);

        /* nobody else gets a unit until a unit is released */
        Sleep(TEST_NO_MORE_ACQUISITIONS_WAIT_MS);
        ASSERT_ARE_EQUAL(int32_t, expected_acquired, interlocked_add(&completed_count, 0));
        ASSERT_IS_TRUE(interlocked_add(&current_holders, 0) <= (int32_t)max_count);
        assert_holders_have_different_units(user_count);

        for (uint32_t i = 0; i < user_count; i++)
        {
            if (test_user_holds_a_unit(&test_users[i]))
            {
                ASSERT_IS_TRUE((uint32_t)interlocked_add(&test_users[i].unit_index, 0) < max_count);
                release_test_user(&test_users[i]);
                released_count++;
                break;
            }
            else
            {
                /* do nothing, this user waits or already released its unit */
            }
        }
    }

    ASSERT_ARE_EQUAL(int32_t, (int32_t)max_count, interlocked_add(&max_holders, 0));
    for (uint32_t i = 0; i < user_count; i++)
    {
        ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[i]));
    }
    wait_for_held_unit_count(test_semaphore_name, max_count, 0);
}

TEST_FUNCTION(robust_semaphores_with_different_names_do_not_share_units)
{
    // arrange
    char other_name[128];
    ASSERT_IS_TRUE(snprintf(other_name, sizeof(other_name), "%s_other", test_semaphore_name) > 0);
    create_test_user(&test_users[0], test_semaphore_name, 1, NULL);
    create_test_user(&test_users[1], other_name, 1, NULL);

    // act
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));

    // cleanup
    release_test_user(&test_users[0]);
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
    wait_for_held_unit_count(other_name, 1, 0);
}

/* the mutexes are opened with the security descriptor by the first user and opened as they are by the other users */
TEST_FUNCTION(robust_semaphores_created_with_a_security_descriptor_share_units)
{
    // arrange
    create_test_user(&test_users[0], test_semaphore_name, 1, "D:P(A;;GA;;;WD)");
    create_test_user(&test_users[1], test_semaphore_name, 1, "D:P(A;;GA;;;WD)");
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(1);
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[1]);

    // act
    release_test_user(&test_users[0]);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));

    // cleanup
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

TEST_FUNCTION(cancel_completes_a_waiting_acquisition_with_CANCELLED_without_taking_a_unit)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(1);
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[1]);

    // act
    robust_semaphore_cancel(test_users[1].robust_semaphore);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, get_result(&test_users[1]));
    ASSERT_ARE_EQUAL(uint32_t, ROBUST_SEMAPHORE_UNIT_INDEX_NONE, (uint32_t)interlocked_add(&test_users[1].unit_index, 0));
    wait_for_held_unit_count(test_semaphore_name, 1, 1);

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

TEST_FUNCTION(waiting_longer_than_timeout_ms_completes_with_TIMEOUT_without_taking_a_unit)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(1);
    double start = timer_global_get_elapsed_ms();

    // act
    acquire_test_user(&test_users[1], 500, ROBUST_SEMAPHORE_NO_TIMEOUT);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_TIMEOUT, get_result(&test_users[1]));
    ASSERT_IS_TRUE(interlocked_add_64(&test_users[1].completed_ms, 0) - (int64_t)start >= 450);
    wait_for_held_unit_count(test_semaphore_name, 1, 1);

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

/* the user never releases its unit, the worker thread releases it after max_hold_ms */
TEST_FUNCTION(holding_a_unit_longer_than_max_hold_ms_gives_it_to_a_waiting_acquisition)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, 500);
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));

    // act
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));
    ASSERT_IS_TRUE(interlocked_add_64(&test_users[1].completed_ms, 0) - interlocked_add_64(&test_users[0].completed_ms, 0) >= 450);

    // cleanup
    /* the unit of user 0 is already released, releasing it again does nothing */
    release_test_user(&test_users[0]);
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

/* max_hold_ms includes the time of the callback, a callback that takes longer gets the unit released as soon as it returns */
TEST_FUNCTION(a_callback_that_runs_longer_than_max_hold_ms_releases_the_unit_when_it_returns)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user_with_callback(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, 100, on_acquire_complete_slow);
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));

    // act
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&test_users[1].slow_callback_returned_at_completion, 0), "the unit was released before the callback returned");

    // cleanup
    release_test_user(&test_users[0]);
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

/* the mutexes are owned by threads, a mutex whose owner thread exited is abandoned */
TEST_FUNCTION(an_abandoned_unit_is_acquired)
{
    // arrange
    create_test_users(1, test_semaphore_name, 1);
    UNIT_OWNER_THREAD_CONTEXT owner_context;
    THREAD_HANDLE owner_thread;
    start_unit_owner_thread(&owner_context, &owner_thread, test_semaphore_name, 0);
    /* the robust semaphore keeps the mutex open, so it stays abandoned after the owner thread exits */
    end_unit_owner_thread(&owner_context, owner_thread);

    // act
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);

    // assert
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_users[0].unit_index, 0));

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

TEST_FUNCTION(a_unit_held_by_a_thread_that_exits_without_releasing_it_is_given_to_a_waiting_acquisition)
{
    // arrange
    UNIT_OWNER_THREAD_CONTEXT owner_context;
    THREAD_HANDLE owner_thread;
    start_unit_owner_thread(&owner_context, &owner_thread, test_semaphore_name, 0);
    create_test_users(1, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[0]);

    // act
    end_unit_owner_thread(&owner_context, owner_thread);

    // assert
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

/* a process that crashes (or is killed) while holding a unit gives it back: an acquisition waiting in another process gets it */
TEST_FUNCTION(a_unit_held_by_a_process_that_is_killed_is_given_to_an_acquisition_waiting_in_another_process)
{
    // arrange
    start_tester_process(test_semaphore_name);
    HANDLE tester_wait_handles[2] = { test_held_event, test_tester_process.hProcess };
    ASSERT_ARE_EQUAL(uint32_t, WAIT_OBJECT_0, (uint32_t)WaitForMultipleObjects(2, tester_wait_handles, FALSE, TEST_TESTER_PROCESS_TIMEOUT_MS), "the tester process did not get the unit");
    wait_for_held_unit_count(test_semaphore_name, 1, 1);

    create_test_users(1, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[0]);

    // act
    ASSERT_IS_TRUE(TerminateProcess(test_tester_process.hProcess, 42));
    ASSERT_ARE_EQUAL(uint32_t, WAIT_OBJECT_0, (uint32_t)WaitForSingleObject(test_tester_process.hProcess, TEST_WAIT_TIMEOUT_MS));

    // assert
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&test_users[0].unit_index, 0));

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
    /* method_cleanup closes the tester process and the job */
}

TEST_FUNCTION(destroying_a_robust_semaphore_that_holds_a_unit_gives_the_unit_to_a_waiting_acquisition)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[1]);

    // act
    (void)interlocked_exchange(&test_users[0].released, 1);
    (void)interlocked_decrement(&current_holders);
    robust_semaphore_destroy(test_users[0].robust_semaphore);
    test_users[0].robust_semaphore = NULL;

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));

    // cleanup
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

TEST_FUNCTION(releasing_a_waiting_acquisition_completes_it_with_CANCELLED_without_taking_a_unit)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(1);
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[1]);

    // act
    robust_semaphore_release(test_users[1].robust_semaphore);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, get_result(&test_users[1]));
    wait_for_held_unit_count(test_semaphore_name, 1, 1);

    /* the next acquisition of the same object gets the unit as soon as it is free */
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    release_test_user(&test_users[0]);
    wait_for_completed_count(3);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));

    // cleanup
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

TEST_FUNCTION(destroying_a_robust_semaphore_that_waits_completes_the_acquisition_with_CANCELLED_without_taking_a_unit)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);
    acquire_test_user(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(1);
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    assert_test_user_waits(&test_users[1]);

    // act
    robust_semaphore_destroy(test_users[1].robust_semaphore);
    test_users[1].robust_semaphore = NULL;

    // assert
    /* the callback is called before robust_semaphore_destroy returns */
    ASSERT_ARE_EQUAL(int32_t, 2, interlocked_add(&completed_count, 0));
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_CANCELLED, get_result(&test_users[1]));
    wait_for_held_unit_count(test_semaphore_name, 1, 1);

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

TEST_FUNCTION(destroying_a_robust_semaphore_from_its_acquire_callback_releases_the_unit)
{
    // arrange
    create_test_users(2, test_semaphore_name, 1);

    // act
    acquire_test_user_with_callback(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT, on_acquire_complete_destroy);

    // assert
    double start = timer_global_get_elapsed_ms();
    while (interlocked_add(&destroyed_from_callback, 0) == 0)
    {
        ASSERT_IS_TRUE(timer_global_get_elapsed_ms() - start < TEST_WAIT_TIMEOUT_MS, "the callback did not destroy the robust semaphore");
        (void)wait_on_address(&destroyed_from_callback, 0, 100);
    }
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    (void)interlocked_exchange(&test_users[0].released, 1);
    (void)interlocked_decrement(&current_holders);

    /* the worker thread released the unit after the callback returned */
    acquire_test_user(&test_users[1], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT);
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[1]));

    // cleanup
    release_test_user(&test_users[1]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

/* the worker thread only lives while there is an acquisition, so an idle robust semaphore has no thread */
TEST_FUNCTION(the_worker_thread_ends_when_the_unit_is_released_and_the_next_acquisition_starts_a_new_one)
{
    // arrange
    create_test_users(1, test_semaphore_name, 1);
    acquire_test_user_with_callback(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT, on_acquire_complete_open_worker_thread);
    wait_for_completed_count(1);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    HANDLE first_worker_thread = get_test_user_worker_thread(&test_users[0]);
    ASSERT_IS_NOT_NULL(first_worker_thread);

    // act
    release_test_user(&test_users[0]);

    // assert
    /* the worker thread ends while the robust semaphore still exists */
    ASSERT_ARE_EQUAL(uint32_t, WAIT_OBJECT_0, (uint32_t)WaitForSingleObject(first_worker_thread, TEST_WAIT_TIMEOUT_MS), "the worker thread did not end after the unit was released");
    close_test_user_worker_thread(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);

    /* the next acquisition of the same robust semaphore starts a new worker thread, which ends too */
    acquire_test_user_with_callback(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT, on_acquire_complete_open_worker_thread);
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    HANDLE second_worker_thread = get_test_user_worker_thread(&test_users[0]);
    ASSERT_IS_NOT_NULL(second_worker_thread);
    release_test_user(&test_users[0]);
    ASSERT_ARE_EQUAL(uint32_t, WAIT_OBJECT_0, (uint32_t)WaitForSingleObject(second_worker_thread, TEST_WAIT_TIMEOUT_MS), "the second worker thread did not end after the unit was released");
    wait_for_held_unit_count(test_semaphore_name, 1, 0);

    // cleanup
    close_test_user_worker_thread(&test_users[0]);
    robust_semaphore_destroy(test_users[0].robust_semaphore);
    test_users[0].robust_semaphore = NULL;
}

/* the worker thread is still running when on_acquire_complete starts a new acquisition, so it takes it instead of a new worker thread */
TEST_FUNCTION(a_new_acquisition_started_from_on_acquire_complete_is_taken_by_the_same_worker_thread)
{
    // arrange
    create_test_users(1, test_semaphore_name, 1);

    // act
    acquire_test_user_with_callback(&test_users[0], ROBUST_SEMAPHORE_NO_TIMEOUT, ROBUST_SEMAPHORE_NO_TIMEOUT, on_acquire_complete_release_and_acquire_again);

    // assert
    wait_for_completed_count(2);
    ASSERT_ARE_EQUAL(int32_t, 0, interlocked_add(&acquire_from_callback_result, 0));
    ASSERT_ARE_EQUAL(ROBUST_SEMAPHORE_ACQUIRE_RESULT, ROBUST_SEMAPHORE_ACQUIRE_OK, get_result(&test_users[0]));
    ASSERT_ARE_EQUAL(int32_t, interlocked_add(&first_worker_thread_id, 0), interlocked_add(&test_users[0].worker_thread_id, 0));

    // cleanup
    release_test_user(&test_users[0]);
    wait_for_held_unit_count(test_semaphore_name, 1, 0);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
