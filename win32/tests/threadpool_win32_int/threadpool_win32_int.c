// Copyright (c) Microsoft. All rights reserved.

#ifdef __cplusplus
#include <cstdlib>
#include <cinttypes>
#include <cmath>
#else
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>
#endif

#include "windows.h"
#include "azure_macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"
#include "threadpool.h"
#include "execution_engine.h"
#include "execution_engine_win32.h"

static TEST_MUTEX_HANDLE test_serialize_mutex;

static void on_open_complete(void* context, THREADPOOL_OPEN_RESULT open_result)
{
    HANDLE* event = (HANDLE*)context;
    (void)SetEvent(*event);
    (void)open_result;
}

static void on_open_complete_do_nothing(void* context, THREADPOOL_OPEN_RESULT open_result)
{
    (void)context;
    (void)open_result;
}

static void work_function(void* context)
{
    volatile LONG64* call_count = (volatile LONG64*)context;
    (void)InterlockedIncrement64(call_count);
    WakeByAddressSingle((PVOID)call_count);
}

typedef struct WAIT_WORK_CONTEXT_TAG
{
    volatile LONG64 call_count;
    HANDLE wait_event;
} WAIT_WORK_CONTEXT;

static void wait_work_function(void* context)
{
    WAIT_WORK_CONTEXT* wait_work_context = (WAIT_WORK_CONTEXT*)context;
    ASSERT_IS_TRUE(WaitForSingleObject(wait_work_context->wait_event, 2000) == WAIT_TIMEOUT);
    (void)InterlockedIncrement64(&wait_work_context->call_count);
    WakeByAddressSingle((PVOID)&wait_work_context->call_count);
}

typedef struct CLOSE_WORK_CONTEXT_TAG
{
    volatile LONG64 call_count;
    THREADPOOL_HANDLE threadpool;
} CLOSE_WORK_CONTEXT;

static void close_work_function(void* context)
{
    CLOSE_WORK_CONTEXT* close_work_context = (CLOSE_WORK_CONTEXT*)context;
    threadpool_close(close_work_context->threadpool);
    (void)InterlockedIncrement64(&close_work_context->call_count);
    WakeByAddressSingle((PVOID)&close_work_context->call_count);
}

typedef struct OPEN_WORK_CONTEXT_TAG
{
    volatile LONG64 call_count;
    THREADPOOL_HANDLE threadpool;
} OPEN_WORK_CONTEXT;

static void open_work_function(void* context)
{
    OPEN_WORK_CONTEXT* open_work_context = (OPEN_WORK_CONTEXT*)context;
    ASSERT_ARE_NOT_EQUAL(int, 0, threadpool_open_async(open_work_context->threadpool, on_open_complete, open_work_context->threadpool));
    (void)InterlockedIncrement64(&open_work_context->call_count);
    WakeByAddressSingle((PVOID)&open_work_context->call_count);
}

BEGIN_TEST_SUITE(threadpool_win32_inttests)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);
}

TEST_SUITE_CLEANUP(suite_cleanup)
{
    TEST_MUTEX_DESTROY(test_serialize_mutex);
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

TEST_FUNCTION(one_work_item_schedule_works)
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);

    // open
    HANDLE open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(open_event);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_open_complete, &open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(open_event, INFINITE) == WAIT_OBJECT_0);

    (void)InterlockedExchange(&call_count, 0);

    // act (schedule one work item)
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));

    // assert
    do
    {
        LONG current_call_count = InterlockedAdd(&call_count, 0);
        if (current_call_count == 1)
        {
            break;
        }

        (void)WaitOnAddress(&call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    // cleanup
    (void)CloseHandle(open_event);
    threadpool_close(threadpool);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

#define N_WORK_ITEMS 100

TEST_FUNCTION(MU_C3(scheduling_, N_WORK_ITEMS, _work_items_works))
{
    // assert
    // create an execution engine
    volatile LONG call_count;
    size_t i;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);

    // open
    HANDLE open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(open_event);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_open_complete, &open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(open_event, INFINITE) == WAIT_OBJECT_0);

    (void)InterlockedExchange(&call_count, 0);

    // act (schedule work items)
    for (i = 0; i < N_WORK_ITEMS; i++)
    {
        ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&call_count));
    }

    // assert
    do
    {
        LONG current_call_count = InterlockedAdd(&call_count, 0);
        if (current_call_count == N_WORK_ITEMS)
        {
            break;
        }

        (void)WaitOnAddress(&call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    // cleanup
    (void)CloseHandle(open_event);
    threadpool_close(threadpool);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(close_while_items_are_scheduled_still_executes_all_items)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 1, 1 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);

    // open
    HANDLE open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(open_event);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_open_complete, &open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(open_event, INFINITE) == WAIT_OBJECT_0);

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange64(&wait_work_context.call_count, 0);

    // schedule one item that waits
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, wait_work_function, (void*)&wait_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&wait_work_context.call_count));

    // call close
    threadpool_close(threadpool);

    SetEvent(wait_work_context.wait_event);

    // assert
    do
    {
        LONG64 current_call_count = InterlockedAdd64(&wait_work_context.call_count, 0);
        if (current_call_count == 2)
        {
            break;
        }

        (void)WaitOnAddress(&wait_work_context.call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    // cleanup
    (void)CloseHandle(open_event);
    (void)CloseHandle(wait_work_context.wait_event);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(close_while_closing_still_executes_the_items)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    // force one thread
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 1, 1 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    CLOSE_WORK_CONTEXT close_work_context;

    // open
    HANDLE open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(open_event);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_open_complete, &open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(open_event, INFINITE) == WAIT_OBJECT_0);

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange64(&wait_work_context.call_count, 0);
    (void)InterlockedExchange64(&close_work_context.call_count, 0);
    close_work_context.threadpool = threadpool;

    // schedule one item that waits, one that calls close and one that does nothing
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, wait_work_function, (void*)&wait_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, close_work_function, (void*)&close_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&wait_work_context.call_count));

    // call close
    threadpool_close(threadpool);

    // set the event, that would trigger a WAIT_OBJECT_0 if close would not wait for all items
    SetEvent(wait_work_context.wait_event);

    // wait for all callbacks to complete
    do
    {
        LONG64 current_call_count = InterlockedAdd64(&close_work_context.call_count, 0);
        if (current_call_count == 1)
        {
            break;
        }

        (void)WaitOnAddress(&close_work_context.call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    do
    {
        LONG64 current_call_count = InterlockedAdd64(&wait_work_context.call_count, 0);
        if (current_call_count == 2)
        {
            break;
        }

        (void)WaitOnAddress(&wait_work_context.call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    // cleanup
    (void)CloseHandle(open_event);
    (void)CloseHandle(wait_work_context.wait_event);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

TEST_FUNCTION(open_while_closing_fails)
{
    // assert
    // create an execution engine
    WAIT_WORK_CONTEXT wait_work_context;
    // force one thread
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 1, 1 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    ASSERT_IS_NOT_NULL(execution_engine);

    // create the threadpool
    THREADPOOL_HANDLE threadpool = threadpool_create(execution_engine);
    OPEN_WORK_CONTEXT open_work_context;

    // open
    HANDLE open_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(open_event);

    ASSERT_ARE_EQUAL(int, 0, threadpool_open_async(threadpool, on_open_complete, &open_event));

    // wait for open to complete
    ASSERT_IS_TRUE(WaitForSingleObject(open_event, INFINITE) == WAIT_OBJECT_0);

    wait_work_context.wait_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    ASSERT_IS_NOT_NULL(wait_work_context.wait_event);

    (void)InterlockedExchange64(&wait_work_context.call_count, 0);
    (void)InterlockedExchange64(&open_work_context.call_count, 0);
    open_work_context.threadpool = threadpool;

    // schedule one item that waits, one that calls close and one that does nothing
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, wait_work_function, (void*)&wait_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, open_work_function, (void*)&open_work_context));
    ASSERT_ARE_EQUAL(int, 0, threadpool_schedule_work(threadpool, work_function, (void*)&wait_work_context.call_count));

    // call close
    threadpool_close(threadpool);

    // set the event, that would trigger a WAIT_OBJECT_0 if close would not wait for all items
    SetEvent(wait_work_context.wait_event);

    // wait for all callbacks to complete
    do
    {
        LONG64 current_call_count = InterlockedAdd64(&open_work_context.call_count, 0);
        if (current_call_count == 1)
        {
            break;
        }

        (void)WaitOnAddress(&open_work_context.call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    do
    {
        LONG64 current_call_count = InterlockedAdd64(&wait_work_context.call_count, 0);
        if (current_call_count == 2)
        {
            break;
        }

        (void)WaitOnAddress(&wait_work_context.call_count, &current_call_count, sizeof(current_call_count), INFINITE);
    } while (1);

    // cleanup
    (void)CloseHandle(open_event);
    (void)CloseHandle(wait_work_context.wait_event);
    threadpool_destroy(threadpool);
    execution_engine_dec_ref(execution_engine);
}

#define CHAOS_THREAD_COUNT 4

typedef struct CHAOS_TEST_DATA_TAG
{
    volatile LONG64 expected_call_count;
    volatile LONG64 executed_work_functions;
    volatile LONG chaos_test_done;
    THREADPOOL_HANDLE threadpool;
} CHAOS_TEST_DATA;

static DWORD WINAPI chaos_thread_func(LPVOID lpThreadParameter)
{
    CHAOS_TEST_DATA* chaos_test_data = (CHAOS_TEST_DATA*)lpThreadParameter;

    while (InterlockedAdd(&chaos_test_data->chaos_test_done, 0) == 0)
    {
        int which_action = rand() * 3 / RAND_MAX;
        switch (which_action)
        {
        case 0:
            // perform an open
            (void)threadpool_open_async(chaos_test_data->threadpool, on_open_complete_do_nothing, NULL);
            break;
        case 1:
            // perform a close
            (void)threadpool_close(chaos_test_data->threadpool);
            break;
        case 2:
            // perform a schedule item
            if (threadpool_schedule_work(chaos_test_data->threadpool, work_function, (void*)&chaos_test_data->executed_work_functions) == 0)
            {
                (void)InterlockedIncrement64(&chaos_test_data->expected_call_count);
            }
            break;
        }
    }

    return 0;
}

TEST_FUNCTION(chaos_knight_test)
{
    // start a number of threads and each of them will do a random action on the threadpool
    EXECUTION_ENGINE_PARAMETERS_WIN32 execution_engine_parameters = { 4, 0 };
    EXECUTION_ENGINE_HANDLE execution_engine = execution_engine_create(&execution_engine_parameters);
    HANDLE thread_handles[CHAOS_THREAD_COUNT];
    size_t i;
    CHAOS_TEST_DATA chaos_test_data;
    chaos_test_data.threadpool = threadpool_create(execution_engine);

    (void)InterlockedExchange64(&chaos_test_data.expected_call_count, 0);
    (void)InterlockedExchange64(&chaos_test_data.executed_work_functions, 0);
    (void)InterlockedExchange(&chaos_test_data.chaos_test_done, 0);

    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        thread_handles[i] = CreateThread(NULL, 0, chaos_thread_func, &chaos_test_data, 0, NULL);
        ASSERT_IS_NOT_NULL(thread_handles[i], "thread %zu failed to start", i);
    }

    // wait for some time
    Sleep(10000);

    (void)InterlockedExchange(&chaos_test_data.chaos_test_done, 1);

    // wait for all threads to complete
    for (i = 0; i < CHAOS_THREAD_COUNT; i++)
    {
        ASSERT_IS_TRUE(WaitForSingleObject(thread_handles[i], INFINITE) == WAIT_OBJECT_0);
    }

    // assert that all scheduled items were executed
    ASSERT_ARE_EQUAL(int64_t, (int64_t)chaos_test_data.expected_call_count, (int64_t)chaos_test_data.executed_work_functions);

    // call close
    threadpool_close(chaos_test_data.threadpool);

    // cleanup
    threadpool_destroy(chaos_test_data.threadpool);
    execution_engine_dec_ref(execution_engine);
}

END_TEST_SUITE(threadpool_win32_inttests)
