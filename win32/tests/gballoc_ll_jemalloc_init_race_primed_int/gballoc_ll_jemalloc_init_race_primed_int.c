// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Regression guard for SRS_GBALLOC_LL_JEMALLOC_02_011 (jemalloc init-time priming).
//
// The suite initializer primes jemalloc single-threaded via gballoc_ll_init. With jemalloc already
// initialized, many threads racing the next allocations no longer hit jemalloc's one-time lazy init
// concurrently, so this race is safe and the test passes deterministically. If the priming in
// gballoc_ll_init regresses, gballoc_ll_init no longer initializes jemalloc and this race crashes.
//
// The companion gballoc_ll_jemalloc_init_race_unprimed_int demonstrates the crash WITHOUT priming.

#include <stddef.h>
#include <stdint.h>

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/interlocked.h"
#include "c_pal/threadapi.h"

#include "c_pal/gballoc_ll.h"
#include "c_pal/timed_test_suite.h"

TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES)

#define RACE_THREAD_COUNT   16
#define RACE_ALLOC_COUNT    256

typedef struct RACE_CONTEXT_TAG
{
    volatile_atomic int32_t* ready_count;
    volatile_atomic int32_t* start_gate;
    int result;
} RACE_CONTEXT;

static int race_allocation(void* arg)
{
    RACE_CONTEXT* context = (RACE_CONTEXT*)arg;

    // announce readiness, then spin until released so every thread allocates at the same instant
    (void)interlocked_increment(context->ready_count);
    while (interlocked_add(context->start_gate, 0) == 0)
    {
    }

    context->result = 0;
    for (uint32_t i = 0; i < RACE_ALLOC_COUNT; i++)
    {
        void* ptr = gballoc_ll_malloc(8 + (i * 13) % 1024);
        if (ptr == NULL)
        {
            context->result = MU_FAILURE;
            break;
        }
        gballoc_ll_free(ptr);
    }
    return context->result;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TIMED_TEST_SUITE_INITIALIZE(TestClassInitialize, TIMED_TEST_DEFAULT_TIMEOUT_MS)
{
    // prime jemalloc single-threaded so the racing threads below do not hit jemalloc's lazy init concurrently
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_init(NULL));
}

TIMED_TEST_SUITE_CLEANUP(TestClassCleanup)
{
    gballoc_ll_deinit();
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(gballoc_ll_malloc_is_thread_safe_when_racing_after_jemalloc_is_primed)
{
    ///arrange
    // a gate that releases every thread simultaneously, plus a counter so the main thread can wait
    // until all threads are parked on the gate before firing (a true simultaneous start)
    volatile_atomic int32_t ready_count;
    volatile_atomic int32_t start_gate;
    (void)interlocked_exchange(&ready_count, 0);
    (void)interlocked_exchange(&start_gate, 0);

    RACE_CONTEXT contexts[RACE_THREAD_COUNT];
    THREAD_HANDLE threads[RACE_THREAD_COUNT];
    for (uint32_t index = 0; index < RACE_THREAD_COUNT; index++)
    {
        contexts[index].ready_count = &ready_count;
        contexts[index].start_gate = &start_gate;
        contexts[index].result = MU_FAILURE;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[index], race_allocation, &contexts[index]));
    }

    // wait until all threads have reached the gate so releasing it is a genuine simultaneous start
    while (interlocked_add(&ready_count, 0) != RACE_THREAD_COUNT)
    {
    }

    ///act
    (void)interlocked_exchange(&start_gate, 1);

    ///assert
    for (uint32_t index = 0; index < RACE_THREAD_COUNT; index++)
    {
        int dont_care;
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[index], &dont_care));
        ASSERT_ARE_EQUAL(int, 0, contexts[index].result);
    }
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
