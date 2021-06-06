// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cinttypes>
#include <cstdlib>
#else
#include <inttypes.h>
#include <stdlib.h>
#endif

#include "macro_utils/macro_utils.h" // IWYU pragma: keep

// IWYU pragma: no_include <wchar.h>
#include "testrunnerswitcher.h"
#include "c_pal/threadapi.h"

#include "c_pal/sysinfo.h"
#include "c_pal/call_once.h"     // for call_once_t
#include "c_pal/interlocked.h"
#include "c_pal/sync.h"
#include "c_pal/lazy_init.h"


TEST_DEFINE_ENUM_TYPE(LAZY_INIT_RESULT, LAZY_INIT_RESULT_VALUES);
TEST_DEFINE_ENUM_TYPE(THREADAPI_RESULT, THREADAPI_RESULT_VALUES);

static TEST_MUTEX_HANDLE test_serialize_mutex;

static int32_t nThreadsForChaos;

static call_once_t lazy_chaos = LAZY_INIT_NOT_DONE;
static volatile_atomic int32_t n_spawned_chaos = 0;
static volatile_atomic int32_t data_chaos = 0;

static int do_init(void* params)
{
    volatile_atomic int32_t* same_as_data_chaos = (volatile_atomic int32_t *)params;
    interlocked_increment(same_as_data_chaos);
    return 0;
}

/*the bollard exists to make sure:
1) that the threds are started (there can be a delay between ThreadAPI_Create and when the thread actually start to execute)
2) be a better alternative to while(interlocked_add != nThreadsForChaos){} - which is extremely detrimental to helgrind
3) and to provide a modicum of "synchronization" between threads (for the purpose of sync spinnig is better, but makes helgrind extremely unhappy)
*/

static volatile_atomic int32_t theBollardIsLowered=0; /*all thrads wait on this to become "1" and then they call "lazy_init". */

static volatile_atomic int32_t lowerTheBollard = 0; /*set to "1" when all threads have executed the last instruction before waiting for the bollard*/

static volatile_atomic int32_t nThreadsbeforeBollard = 0;

static void same_as_interlocked_hl_wait_for_value(int32_t volatile_atomic* address, int32_t value, uint32_t milliseconds)
{
    do
    {
        int32_t current_value;
        current_value = interlocked_add(address, 0);
        if (current_value == value)
        {
            break;
        }

        ASSERT_IS_TRUE(wait_on_address(address, current_value, milliseconds));

    } while (1);
}

static int chaosThread(
    void* lpParameter
)
{
    (void)lpParameter;

    if (interlocked_increment(&nThreadsbeforeBollard) == nThreadsForChaos)
    {
        (void)interlocked_exchange(&lowerTheBollard, 1);
        wake_by_address_single(&lowerTheBollard);
    }

    same_as_interlocked_hl_wait_for_value(&theBollardIsLowered, 1, UINT32_MAX);

    LAZY_INIT_RESULT result;

    ///act
    result = lazy_init(&lazy_chaos, do_init, (void*)&data_chaos);

    ///assert
    ASSERT_ARE_EQUAL(LAZY_INIT_RESULT, LAZY_INIT_OK, result);
    ASSERT_ARE_EQUAL(int32_t, 1, interlocked_add(&data_chaos, 0));

    return 0;
}

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(suite_init)
{
    test_serialize_mutex = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(test_serialize_mutex);

    nThreadsForChaos = sysinfo_get_processor_count();
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
    int32_t i;
    THREAD_HANDLE* threads = malloc(nThreadsForChaos * sizeof(THREAD_HANDLE));
    ASSERT_IS_NOT_NULL(threads);


    for (i = 0; i < nThreadsForChaos; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Create(&threads[i], chaosThread, NULL));
        ASSERT_IS_NOT_NULL(threads[i]);

    }

    same_as_interlocked_hl_wait_for_value(&lowerTheBollard, 1, UINT32_MAX);

    interlocked_exchange(&theBollardIsLowered, 1);
    wake_by_address_all(&theBollardIsLowered);

    for (i = 0; i < nThreadsForChaos; i++)
    {
        ASSERT_ARE_EQUAL(THREADAPI_RESULT, THREADAPI_OK, ThreadAPI_Join(threads[i], NULL));
    }

    ///assert - all done in the threads themselves
    
    ///clean
    free(threads);
}


END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
