// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstdint>
#include <cstdlib>
#else
#include <stdint.h>
#include <stdlib.h>
#endif

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"
#include "c_pal/threadapi.h"

#include "c_pal/call_once.h"     // for call_once_t
#include "c_pal/interlocked.h"
#include "c_pal/lazy_init.h"

TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

static TEST_MUTEX_HANDLE test_serialize_mutex;

#define N_THREADS_FOR_CHAOS 16

static call_once_t lazy_chaos = LAZY_INIT_NOT_DONE;
static volatile_atomic int32_t n_spawned_chaos = 0;
static volatile_atomic int32_t data_chaos = 0;

static int do_init(void* params)
{
    volatile_atomic int32_t* same_as_data_chaos = (volatile_atomic int32_t *)params;
    interlocked_increment(same_as_data_chaos);
    return 0;
}

static int chaosThread(
    void* lpParameter
)
{
    (void)lpParameter;
    (void)interlocked_increment(&n_spawned_chaos);
    while (interlocked_add(&n_spawned_chaos, 0) != N_THREADS_FOR_CHAOS)
    {
        /*spin*/
    }

    LAZY_INIT_RESULT result;

    ///act
    result = lazy_init(&lazy_chaos, do_init, (void*)&data_chaos);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&data_chaos, 0));

    return 0;
}

BEGIN_TEST_SUITE(lazy_init_inttests)

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

TEST_FUNCTION(lazy_init_chaos_knight)
{
    /*this test spawns 10 threads. The threads are all spinning until they are all created, then they do lazy_init*/
    /*lazy init increments  variable*/
    /*it is expected that 1) the variable gets to "1" 2) all threads complete execution*/

    ///arrange
    size_t i;
    THREAD_HANDLE threads[N_THREADS_FOR_CHAOS];

    for (i = 0; i < N_THREADS_FOR_CHAOS; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[i], chaosThread, NULL));
        ASSERT_IS_NOT_NULL(threads[i]);

    }

    for (i = 0; i < N_THREADS_FOR_CHAOS; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[i], NULL));
    }

    ///assert - all done in the threads themselves
    
    ///clean
}


END_TEST_SUITE(lazy_init_inttests)
