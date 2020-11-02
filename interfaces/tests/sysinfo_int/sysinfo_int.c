//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h"
#include "c_pal/sysinfo.h"

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(timer_int)

TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));

    g_testByTest = TEST_MUTEX_CREATE();
    ASSERT_IS_NOT_NULL(g_testByTest);
}

TEST_SUITE_CLEANUP(b)
{
    TEST_MUTEX_DESTROY(g_testByTest);
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(c)
{
    if (TEST_MUTEX_ACQUIRE(g_testByTest))
    {
        ASSERT_FAIL("our mutex is ABANDONED. Failure in test framework");
    }
}

TEST_FUNCTION_CLEANUP(d)
{
    TEST_MUTEX_RELEASE(g_testByTest);
}

/* sysinfo_get_processor_count */

TEST_FUNCTION(sysinfo_get_processor_count_returns_processor_count)
{
    ///arrange

    ///act
    uint32_t proc_count = sysinfo_get_processor_count();

    ///assert
    ASSERT_ARE_NOT_EQUAL(uint32_t, 0, proc_count);
}

END_TEST_SUITE(timer_int)
