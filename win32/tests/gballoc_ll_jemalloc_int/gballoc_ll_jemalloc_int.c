// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

#include <windows.h>
#include <psapi.h>

#include "macro_utils/macro_utils.h"
#include "testrunnerswitcher.h"

#include "c_pal/gballoc_ll.h"
#include "c_pal/threadapi.h"

#include "jemalloc/jemalloc.h"

#define DECAY_MS 50000
#define MAX_ARENAS 4

static int64_t default_dirty_decay_ms;
static int64_t default_muzzy_decay_ms;

BEGIN_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)

TEST_SUITE_INITIALIZE(TestClassInitialize)
{
    // Configure jemalloc to have 4 arenas(hence total max limit for number of arenas = 5, 4 normal + 1 huge)
    je_malloc_conf = "narenas:4";

    size_t decay_size = sizeof(int64_t);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.dirty_decay_ms", &default_dirty_decay_ms, &decay_size, NULL, 0));
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.muzzy_decay_ms", &default_muzzy_decay_ms, &decay_size, NULL, 0));
}

TEST_SUITE_CLEANUP(TestClassCleanup)
{
}

TEST_FUNCTION_INITIALIZE(TestMethodInitialize)
{
}

TEST_FUNCTION_CLEANUP(TestMethodCleanup)
{
}

TEST_FUNCTION(gballoc_ll_init_works)
{
    ///arrange

    ///act
    gballoc_ll_init(NULL);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_deinit_works)
{
    ///arrange

    ///act
    gballoc_ll_deinit();

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_malloc_works)
{
    // arrange

    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc(1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_1MB_works)
{
    // arrange

    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc(1024 * 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_2_works)
{
    // arrange

    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_2(1, 1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_2_1MB_works)
{
    // arrange

    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_2(1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_flex_works)
{
    // arrange

    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_flex(1, 1, 1);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1 + 1 * 1); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_malloc_flex_1MB_works)
{
    // arrange

    ///act (1)
    unsigned char* ptr = gballoc_ll_malloc_flex(1024, 1024, 1024);

    ///assert (1)
    ASSERT_IS_NOT_NULL(ptr);

    ///act(2)
    (void)memset(ptr, 0, 1024 + 1024 * 1024); /*can be written*/

    ///assert (2) - doesn't crash

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_free_works)
{
    ///arrange
    unsigned char* ptr = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr);

    ///act 
    gballoc_ll_free(ptr);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_realloc_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_ll_realloc(ptr1, 2);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}

TEST_FUNCTION(gballoc_ll_realloc_2_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_ll_malloc(1);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_ll_realloc_2(ptr1, 1, 2);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}

TEST_FUNCTION(gballoc_ll_realloc_flex_works)
{
    ///arrange
    unsigned char* ptr1 = gballoc_ll_malloc_flex(4, 10, 8);
    ASSERT_IS_NOT_NULL(ptr1);
    unsigned char* ptr2;

    ///act 
    ptr2 = gballoc_ll_realloc_flex(ptr1, 4, 20, 8);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr2);

    ///clean
    gballoc_ll_free(ptr2);
}


TEST_FUNCTION(gballoc_ll_calloc_works)
{
    ///arrange
    unsigned char* ptr;

    ///act 
    ptr = gballoc_ll_calloc(1, 1);

    ///assert - doesn't crash
    ASSERT_IS_NOT_NULL(ptr);
    ASSERT_IS_TRUE(0 == ptr[0]);

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_size_works)
{
    /// arrange
    void* ptr = gballoc_ll_malloc(4);
    ASSERT_IS_NOT_NULL(ptr);

    size_t size;

    ///act
    size = gballoc_ll_size(ptr);

    ///assert - this is less than ideal, but the original size asked to be malloc'd is lost
    ASSERT_IS_TRUE(size>=4);

    ///clean
    gballoc_ll_free(ptr);
}

TEST_FUNCTION(gballoc_ll_print_stats_works)
{
    /// arrange
    void* ptr = gballoc_ll_malloc(4);
    ASSERT_IS_NOT_NULL(ptr);

    ///act
    gballoc_ll_print_stats();

    ///assert - this is less than ideal, but the original size asked to be malloc'd is lost

    ///clean
    gballoc_ll_free(ptr);
}

static void assert_set_option_works_for_decay(const char* option_name, int64_t decay_ms, uint32_t narenas)
{
    char command[32];
    int snprintf_result;
    int64_t verify_decay_ms;
    size_t decay_ms_size = sizeof(decay_ms);

    // Arena 1 does not exist, so je_mallctl should return EFAULT
    for (uint32_t i = 0; i < narenas; i++)
    {
        snprintf_result = snprintf(command, sizeof(command), "arena.%" PRIu32 ".%s_ms", i, option_name);
        ASSERT_IS_TRUE(snprintf_result > 0 && snprintf_result < sizeof(command), "snprintf failed");

        if (i == 1)
        {
            // the arena at index 1 does not exist, so this should fail with EFAULT
            ASSERT_ARE_EQUAL(int, EFAULT, je_mallctl(command, &verify_decay_ms, &decay_ms_size, NULL, 0));
        }
        else
        {
            ASSERT_ARE_EQUAL(int, 0, je_mallctl(command, &verify_decay_ms, &decay_ms_size, NULL, 0));
            ASSERT_ARE_EQUAL(int64_t, decay_ms, verify_decay_ms);
        }
    }
}

static void setup_arenas_for_set_option(uint32_t expected_narenas, void* ptr[])
{
    // verify the number of arenas
    uint32_t narenas;
    size_t narenas_size = sizeof(narenas);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("opt.narenas", &narenas, &narenas_size, NULL, 0));
    ASSERT_ARE_EQUAL(uint32_t, expected_narenas, narenas);

    ptr[0] = gballoc_ll_malloc(4); // this malloc will happen in the default arena since this thread is linked to arena 0 by default
    ASSERT_IS_NOT_NULL(ptr[0]);

    // Initialize all the arenas except first(arena at index 1) after the default arena, so that we can verify that the decay is set for all the arenas except arena at index 1
    uint32_t arena_id;
    for (uint32_t i = 2; i < narenas; i++)
    {
        arena_id = i; // assign thread to arena i
        ASSERT_ARE_EQUAL(int, 0, je_mallctl("thread.arena", NULL, NULL, &arena_id, sizeof(arena_id)));

        ptr[i] = gballoc_ll_malloc(4); // this malloc will happen in the arena i
        ASSERT_IS_NOT_NULL(ptr[i]);
    }

    arena_id = 0; // assign thread back to default arena
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("thread.arena", NULL, NULL, &arena_id, sizeof(arena_id)));
}

TEST_FUNCTION(gballoc_ll_set_option_works_for_dirty_decay)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;
    void* ptr[MAX_ARENAS] = { NULL };

    setup_arenas_for_set_option(expected_narenas, ptr);

    int result;
    int64_t decay_ms = DECAY_MS;
    size_t decay_ms_size = sizeof(decay_ms);
    int64_t verify_decay_ms;
    int64_t old_decay_ms;

    // Retrieve the old decay value
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.dirty_decay_ms", &old_decay_ms, &decay_ms_size, NULL, 0));
    ASSERT_ARE_NOT_EQUAL(int64_t, decay_ms, old_decay_ms);

    ///act
    result = gballoc_ll_set_option("dirty_decay", &decay_ms);

    ///assert - doesn't crash
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.dirty_decay_ms", &verify_decay_ms, &decay_ms_size, NULL, 0));
    ASSERT_ARE_EQUAL(int64_t, decay_ms, verify_decay_ms);

    // verify that all the arenas have the decay set except arena 1 and last arena(reserved for huge arena)
    assert_set_option_works_for_decay("dirty_decay", decay_ms, expected_narenas);

    ///clean
    for (uint32_t i = 0; i < expected_narenas; i++)
    {
        gballoc_ll_free(ptr[i]);
    }
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_works_for_muzzy_decay)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;
    void* ptr[MAX_ARENAS] = { NULL };

    setup_arenas_for_set_option(expected_narenas, ptr);

    int result;
    int64_t decay_ms = DECAY_MS;
    size_t decay_ms_size = sizeof(decay_ms);
    int64_t verify_decay_ms;
    int64_t old_decay_ms;

    // Retrieve the old decay value
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.muzzy_decay_ms", &old_decay_ms, &decay_ms_size, NULL, 0));
    ASSERT_ARE_NOT_EQUAL(int64_t, decay_ms, old_decay_ms);

    ///act
    result = gballoc_ll_set_option("muzzy_decay", &decay_ms);

    ///assert - doesn't crash
    ASSERT_ARE_EQUAL(int, 0, result);
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("arenas.muzzy_decay_ms", &verify_decay_ms, &decay_ms_size, NULL, 0));
    ASSERT_ARE_EQUAL(int64_t, decay_ms, verify_decay_ms);

    // verify that all the arenas have the decay set except arena 1 and last arena(reserved for huge arena)
    assert_set_option_works_for_decay("muzzy_decay", decay_ms, expected_narenas);

    ///clean
    for (uint32_t i = 0; i < expected_narenas; i++)
    {
        gballoc_ll_free(ptr[i]);
    }
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

// Returns the working set size of the current process
static int get_working_set_size(size_t* working_set_size)
{
    PROCESS_MEMORY_COUNTERS memCounters;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memCounters, sizeof(memCounters)))
    {
        *working_set_size = memCounters.WorkingSetSize;
        return 0;
    }
    else
    {
        LogLastError("GetProcessMemoryInfo failed");
        return -1;
    }
}

// This test performs the following steps:
// 1. Allocates a lot of memory to dirty the pages
// 2. Sets the decay time for dirty or muzzy pages
// 3. Frees all the allocations to free the pages
// 4. Sleeps for a while to let the decay time elapse
// 5. Forces decay for all arenas
// 6. Sleeps for a while to let the pages be purged
// 7. Verifies that the working set size after decay is as expected
//    - if decay time is high, then the working set size after decay should be more than the residual_working_set_size_percent due to slow purging
//    - if decay time is low, then the working set size after decay should be less than the residual_working_set_size_percent due to fast purging
static void gballoc_ll_set_option_decay_check_working_set_size(uint32_t num_allocations, size_t alloc_size, int64_t decay_ms, uint32_t expected_narenas, uint32_t residual_working_set_size_percent, bool is_percent_max, uint32_t sleep_time_ms, bool is_dirty)
{
    // do a lot of allocations to dirty the pages
    void** ptr = gballoc_ll_malloc(num_allocations * sizeof(void*));
    for (uint32_t i = 0; i < num_allocations; i++)
    {
        ptr[i] = gballoc_ll_malloc(alloc_size);
        ASSERT_IS_NOT_NULL(ptr[i]);
        memset(ptr[i], 0xA5, alloc_size);
    }

    size_t working_set_before;
    int result= get_working_set_size(&working_set_before);
    ASSERT_ARE_EQUAL(int, 0, result);
    LogInfo("Working set size before freeing allocations: %zu bytes\n", working_set_before);

    // set the decay time
    if (is_dirty)
    {
        result = gballoc_ll_set_option("dirty_decay", &decay_ms);
        ASSERT_ARE_EQUAL(int, 0, result);
    }
    else
    {
        // set dirty decay to a low value so as to avoid retaining the dirty pages for long (this ensures that pages are moved to muzzy sooner)
        int64_t low_dirty_decay_ms = 1;
        result = gballoc_ll_set_option("dirty_decay", &low_dirty_decay_ms);
        ASSERT_ARE_EQUAL(int, 0, result);
        result = gballoc_ll_set_option("muzzy_decay", &decay_ms);
        ASSERT_ARE_EQUAL(int, 0, result);
    }

    // free all the allocations
    for (uint32_t i = 0; i < num_allocations; i++)
    {
        gballoc_ll_free(ptr[i]);
    }
    gballoc_ll_free(ptr);

    // sleep for a while for decay time to elapse
    for (uint32_t i = 0; i < 20; i++)
    {
        ThreadAPI_Sleep(sleep_time_ms / 20);
    }

    char command[64];
    // force decay for all arenas, this will free all the dirty or muzzy pages that have been retained for longer than the decay time
    for (uint32_t i = 0; i < expected_narenas; i++)
    {
        snprintf(command, sizeof(command), "arena.%" PRIu32 ".decay", i);
        ASSERT_ARE_EQUAL(int, 0, je_mallctl(command, NULL, NULL, NULL, 0));
    }

    // sleep for a while to let the pages be purged
    for (uint32_t i = 0; i < 20; i++)
    {
        ThreadAPI_Sleep(sleep_time_ms / 20);
    }

    size_t working_set_after;
    result = get_working_set_size(&working_set_after);
    ASSERT_ARE_EQUAL(int, 0, result);
    LogInfo("Working set size after freeing allocations and decay: %zu bytes\n", working_set_after);
    
    if (is_percent_max)
    {
        // If the decay time is high, then the working set size after decay should also remain high
        ASSERT_IS_TRUE(working_set_after <= (working_set_before * residual_working_set_size_percent / 100));
    }
    else
    {
        // If the decay time is low, then the working set size after decay should be low
        ASSERT_IS_TRUE(working_set_after >= (working_set_before * residual_working_set_size_percent / 100));
    }
}

TEST_FUNCTION(gballoc_ll_set_option_check_wss_with_dirty_decay_1_second)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 1000;

    // decay takes some time to purge completely, hence the tolerance of 40%(includes metadata too which is never purged)
    uint32_t residual_working_set_size_percentage = 40;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_working_set_size(num_allocations, alloc_size, decay_ms, expected_narenas, residual_working_set_size_percentage, true, sleep_time_ms, true);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_wss_with_dirty_decay_5_minutes)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 300000;
    uint32_t residual_working_set_size_percentage = 90;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_working_set_size(num_allocations, alloc_size, decay_ms, expected_narenas, residual_working_set_size_percentage, false, sleep_time_ms, true);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_wss_with_dirty_decay_minus_one)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = -1;
    uint32_t residual_working_set_size_percentage = 90;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_working_set_size(num_allocations, alloc_size, decay_ms, expected_narenas, residual_working_set_size_percentage, false, sleep_time_ms, true);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_wss_with_muzzy_decay_1_second)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 1000;

    // decay takes some time to purge completely, hence the tolerance of 40%(includes metadata too which is never purged)
    uint32_t residual_muzzy_pages_percent = 40;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_working_set_size(num_allocations, alloc_size, decay_ms, expected_narenas, residual_muzzy_pages_percent, true, sleep_time_ms, false);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_wss_with_muzzy_decay_5_minutes)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 300000;
    uint32_t residual_muzzy_pages_percent = 90;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_working_set_size(num_allocations, alloc_size, decay_ms, expected_narenas, residual_muzzy_pages_percent, false, sleep_time_ms, false);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_wss_with_muzzy_decay_minus_one)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = -1;
    uint32_t residual_muzzy_pages_percent = 90;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_working_set_size(num_allocations, alloc_size, decay_ms, expected_narenas, residual_muzzy_pages_percent, false, sleep_time_ms, false);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
