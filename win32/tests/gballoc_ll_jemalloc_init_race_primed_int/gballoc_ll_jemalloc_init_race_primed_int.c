// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Gated regression guard for the jemalloc init-time priming fix. Each spawned child primes jemalloc via
// gballoc_ll_init before racing its first allocation, so every child finishes and returns 0 and this
// test passes. The shared driver lives in gballoc_ll_jemalloc_init_race_common.c; the ONLY difference
// from gballoc_ll_jemalloc_init_race_unprimed_int is the prime argument passed below.

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

TEST_FUNCTION(gballoc_ll_first_allocation_is_thread_safe_when_jemalloc_is_primed)
{
    // prime == true: every spawned child primes jemalloc before racing, so none should crash or hang
    gballoc_ll_jemalloc_init_race_run(true);
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
