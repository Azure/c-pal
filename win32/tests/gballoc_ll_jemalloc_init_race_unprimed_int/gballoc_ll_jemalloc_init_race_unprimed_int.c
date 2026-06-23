// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Demonstration of the defect fixed by SRS_GBALLOC_LL_JEMALLOC_02_011. Spawned children do NOT prime
// jemalloc, so racing the process's first allocation hits jemalloc's lazy init concurrently and a
// child crashes or hangs almost immediately. The shared driver spawns children until one fails, which
// fails this test; it is registered WILL_FAIL (CTest inverts the expected failure to a pass). The
// shared driver lives in gballoc_ll_jemalloc_init_race_common.c; the ONLY difference from
// gballoc_ll_jemalloc_init_race_primed_int is the prime argument passed below.

#include "macro_utils/macro_utils.h"

#include "testrunnerswitcher.h"

#include "c_pal/timed_test_suite.h"

#include "gballoc_ll_jemalloc_init_race_common.h"

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TIMED_TEST_SUITE_INITIALIZE(TestClassInitialize, TIMED_TEST_DEFAULT_TIMEOUT_MS)
{
}

TIMED_TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(gballoc_ll_first_allocation_race_without_priming_is_hunted_until_it_fails)
{
    // prime == false: spawned children do not prime, so one crashes/hangs almost immediately
    gballoc_ll_jemalloc_init_race_run(false);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
