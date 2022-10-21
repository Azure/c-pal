// Copyright (c) Microsoft. All rights reserved.


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "macro_utils/macro_utils.h" // IWYU pragma: keep
#include "testrunnerswitcher.h"

#include "umock_c/umock_c.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/threadpool.h"
#include "c_pal/threadapi.h"
#include "c_pal/interlocked.h"

#define INVALID_SOCKET  -1

static TEST_MUTEX_HANDLE test_serialize_mutex;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);

    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(method_init)
{
    if (TEST_MUTEX_ACQUIRE(test_serialize_mutex))
    {
        ASSERT_FAIL("Could not acquire test serialization mutex.");
    }
}

TEST_FUNCTION_CLEANUP(method_cleanup)
{
    TEST_MUTEX_RELEASE(test_serialize_mutex);
}

static void threadpool_task_wait_20_sec(void* parameter)
{
    printf("Running task from thread 0x%0x\n", ThreadAPI_GetCurrentId());

    volatile_atomic uint32_t* thread_counter = (volatile_atomic int32_t*)parameter;
    ThreadAPI_Sleep(20);
    (void)interlocked_increment(thread_counter);
}

TEST_FUNCTION(create_threadpool_1_threads_idle_pool)
{
    // assert
    uint32_t max_thread_count = 10;
    volatile_atomic uint32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(max_thread_count);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create 1 thread pool
    ASSERT_ARE_EQUAL(int, 0, threadpool_add_task(threadpool, threadpool_task_wait_20_sec, (void*)&thread_counter));

    // Wait till
    while (interlocked_add(&thread_counter, 0) != 2);

    // Now let's create a few more threads

    // assert

    // cleanup
    threadpool_destroy(threadpool);
}
#if 0
TEST_FUNCTION(threadpool_create_20_threads_with_max_10_threads_defined)
{
    // assert
    uint32_t max_thread_count = 10;
    volatile_atomic uint32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(max_thread_count);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create double the amount of threads that is the max
    for (size_t index = 0; index < max_thread_count*2; index++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_add_task(threadpool, threadpool_task_wait_20_sec, (void*)&thread_counter));
    }

    // assert
    while (interlocked_add(&thread_counter, 0) != max_thread_count*2);

    // cleanup
    threadpool_destroy(threadpool);
}

TEST_FUNCTION(threadpool_chaos_knight)
{
    // assert
    uint32_t max_thread_count = 32;
    volatile_atomic uint32_t thread_counter = 0;

    THREADPOOL_HANDLE threadpool = threadpool_create(max_thread_count);
    ASSERT_IS_NOT_NULL(threadpool);

    // Create a 1000 threads to be added to the threadpool 
    // with multiple sleeps between creating the thread and different
    // Thread duration lengths

    // assert
    while (interlocked_add(&thread_counter, 0) != max_thread_count*2);

    // cleanup
    threadpool_destroy(threadpool);
}
#endif
END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
