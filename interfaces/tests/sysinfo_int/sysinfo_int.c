//Copyright(c) Microsoft.All rights reserved.
//Licensed under the MIT license. See LICENSE file in the project root for full license information.


#include <stddef.h>
#include <stdint.h>


#include "testrunnerswitcher.h"

#include "c_pal/gballoc_hl.h"
#include "c_pal/gballoc_hl_redirect.h" // IWYU pragma: keep
#include "c_pal/sysinfo.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(a)
{
    ASSERT_ARE_EQUAL(int, 0, gballoc_hl_init(NULL, NULL));
}

TEST_SUITE_CLEANUP(b)
{
    gballoc_hl_deinit();
}

TEST_FUNCTION_INITIALIZE(c)
{
}

TEST_FUNCTION_CLEANUP(d)
{
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

/* Tests_SRS_SYSINFO_01_002: [ If any error occurs, sysinfo_get_processor_count shall return 0. ]*/
/* Can't really be induced on "any" platform, tested independently for each psupported platform */

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
