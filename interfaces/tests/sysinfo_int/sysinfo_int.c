//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stddef.h>
#include <stdint.h>
#endif

#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/sysinfo.h"

static TEST_MUTEX_HANDLE g_testByTest;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

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

/* Tests_SRS_SYSINFO_01_001: [ sysinfo_get_processor_count shall obtain the processor count as reported by the operating system. ]*/
TEST_FUNCTION(sysinfo_get_processor_count_returns_processor_count)
{
    ///arrange

    ///act
    uint32_t proc_count = sysinfo_get_processor_count();

    ///assert
    ASSERT_ARE_NOT_EQUAL(uint32_t, 0, proc_count);
}

/* Tests_SRS_SYSINFO_01_002: [ If any error occurs, `sysinfo_get_processor_count` shall return 0. ]*/
/* Can't really be induced on "any" platform, tested independently for each psupported platform */

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
