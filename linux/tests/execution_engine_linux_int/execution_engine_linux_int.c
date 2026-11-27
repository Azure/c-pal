// Copyright (C) Microsoft Corporation. All rights reserved.

#include <inttypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/threadapi.h"
#include "c_pal/timer.h"

#include "c_pal/execution_engine.h"
#include "c_pal/execution_engine_linux.h"

#define WAIT_TIMEOUT_MS 10000

typedef struct RELEASE_CONTEXT_TAG
{
    EXECUTION_ENGINE_HANDLE execution_engine;
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work;
    EXECUTION_ENGINE_LINUX_TIMER_HANDLE timer;
    bool released;
} RELEASE_CONTEXT;

// the threads of an execution engine torn down by its manager thread are not joined by the tests,
// so the tests synchronize with them using a mutex and a condition variable, which valgrind tools understand
static pthread_mutex_t release_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t release_cond = PTHREAD_COND_INITIALIZER;
static RELEASE_CONTEXT release_context;

static uint32_t get_thread_count(void)
{
    uint32_t result = 0;
    char line[256];
    FILE* status = fopen("/proc/self/status", "r");
    ASSERT_IS_NOT_NULL(status);

    while (fgets(line, sizeof(line), status) != NULL)
    {
        uint32_t thread_count;
        if (sscanf(line, "Threads: %" SCNu32 "", &thread_count) == 1)
        {
            result = thread_count;
        }
        else
        {
            // another field
        }
    }

    (void)fclose(status);
    return result;
}

static bool wait_for_thread_count(uint32_t expected_thread_count)
{
    double start = timer_global_get_elapsed_ms();
    uint32_t thread_count = get_thread_count();

    while ((thread_count != expected_thread_count) && (timer_global_get_elapsed_ms() - start < WAIT_TIMEOUT_MS))
    {
        ThreadAPI_Sleep(10);
        thread_count = get_thread_count();
    }

    return (thread_count == expected_thread_count);
}

static void mark_released(RELEASE_CONTEXT* release_context)
{
    ASSERT_ARE_EQUAL(int, 0, pthread_mutex_lock(&release_lock));
    release_context->released = true;
    ASSERT_ARE_EQUAL(int, 0, pthread_cond_broadcast(&release_cond));
    ASSERT_ARE_EQUAL(int, 0, pthread_mutex_unlock(&release_lock));
}

static bool wait_for_released(void)
{
    struct timespec deadline;
    ASSERT_ARE_EQUAL(int, 0, clock_gettime(CLOCK_REALTIME, &deadline));
    deadline.tv_sec += WAIT_TIMEOUT_MS / 1000;

    ASSERT_ARE_EQUAL(int, 0, pthread_mutex_lock(&release_lock));
    int wait_result = 0;
    while (!release_context.released && (wait_result == 0))
    {
        wait_result = pthread_cond_timedwait(&release_cond, &release_lock, &deadline);
    }
    bool result = release_context.released;
    ASSERT_ARE_EQUAL(int, 0, pthread_mutex_unlock(&release_lock));

    return result;
}

static void reset_release_context(void)
{
    ASSERT_ARE_EQUAL(int, 0, pthread_mutex_lock(&release_lock));
    release_context.execution_engine = NULL;
    release_context.work = NULL;
    release_context.timer = NULL;
    release_context.released = false;
    ASSERT_ARE_EQUAL(int, 0, pthread_mutex_unlock(&release_lock));
}

static void close_work_and_release_execution_engine(void* context)
{
    RELEASE_CONTEXT* release_context = context;
    execution_engine_linux_close_work(release_context->work);
    execution_engine_dec_ref(release_context->execution_engine);
    mark_released(release_context);
}

static void close_timer_and_release_execution_engine(void* context)
{
    RELEASE_CONTEXT* release_context = context;
    execution_engine_linux_close_timer(release_context->timer);
    execution_engine_dec_ref(release_context->execution_engine);
    mark_released(release_context);
}

static void do_nothing(void* context)
{
    (void)context;
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

TEST_FUNCTION_INITIALIZE(init)
{
}

TEST_FUNCTION_CLEANUP(cleanup)
{
}

TEST_FUNCTION(releasing_the_last_reference_from_another_thread_waits_for_the_threads_of_the_execution_engine_to_exit)
{
    ///arrange
    uint32_t thread_count_before = get_thread_count();
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { .min_thread_count = 4, .max_thread_count = 4 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);
    EXECUTION_ENGINE_LINUX_WORK_HANDLE work = execution_engine_linux_create_work(execution_engine, do_nothing, NULL);
    ASSERT_IS_NOT_NULL(work);
    execution_engine_linux_submit_work(work);
    execution_engine_linux_wait_for_work(work, false);
    execution_engine_linux_close_work(work);

    // 4 worker threads and the manager thread
    ASSERT_ARE_EQUAL(uint32_t, thread_count_before + 5, get_thread_count());

    ///act
    execution_engine_dec_ref(execution_engine);

    ///assert
    ASSERT_ARE_EQUAL(uint32_t, thread_count_before, get_thread_count());
}

TEST_FUNCTION(releasing_the_last_reference_from_a_work_function_of_the_execution_engine_tears_it_down)
{
    ///arrange
    uint32_t thread_count_before = get_thread_count();
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { .min_thread_count = 2, .max_thread_count = 0 };
    reset_release_context();
    release_context.execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(release_context.execution_engine);
    release_context.work = execution_engine_linux_create_work(release_context.execution_engine, close_work_and_release_execution_engine, &release_context);
    ASSERT_IS_NOT_NULL(release_context.work);

    ///act
    execution_engine_linux_submit_work(release_context.work);

    ///assert
    ASSERT_IS_TRUE(wait_for_released(), "the work function did not release the execution engine");
    ASSERT_IS_TRUE(wait_for_thread_count(thread_count_before), "the threads of the execution engine did not exit");
}

TEST_FUNCTION(releasing_the_last_reference_from_a_timer_work_function_of_the_execution_engine_tears_it_down)
{
    ///arrange
    uint32_t thread_count_before = get_thread_count();
    EXECUTION_ENGINE_PARAMETERS execution_engine_parameters = { .min_thread_count = 1, .max_thread_count = 0 };
    reset_release_context();
    release_context.execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(release_context.execution_engine);
    release_context.timer = execution_engine_linux_create_timer(release_context.execution_engine, close_timer_and_release_execution_engine, &release_context);
    ASSERT_IS_NOT_NULL(release_context.timer);

    ///act
    execution_engine_linux_set_timer(release_context.timer, 0, 0);

    ///assert
    ASSERT_IS_TRUE(wait_for_released(), "the work function did not release the execution engine");
    ASSERT_IS_TRUE(wait_for_thread_count(thread_count_before), "the threads of the execution engine did not exit");
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
