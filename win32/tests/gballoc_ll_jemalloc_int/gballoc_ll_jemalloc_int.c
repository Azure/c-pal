// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>
#include <stddef.h>

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
    ///act
    gballoc_ll_init(NULL);

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_deinit_works)
{
    ///act
    gballoc_ll_deinit();

    ///assert - doesn't crash
}

TEST_FUNCTION(gballoc_ll_malloc_works)
{
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

static void gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(uint32_t num_allocations, size_t alloc_size, int64_t decay_ms, uint32_t expected_narenas, uint32_t residual_pages_percent, bool is_percent_max, uint32_t sleep_time_ms, bool is_dirty)
{
    // do a lot of allocations to dirty the pages
    void** ptr = gballoc_ll_malloc(num_allocations * sizeof(void*));
    for (uint32_t i = 0; i < num_allocations; i++)
    {
        ptr[i] = gballoc_ll_malloc(alloc_size);
        ASSERT_IS_NOT_NULL(ptr[i]);
        memset(ptr[i], 0xA5, alloc_size);
    }

    int result;

    // set the decay time
    if (is_dirty)
    {
        result = gballoc_ll_set_option("dirty_decay", &decay_ms);
    }
    else
    {
        // set dirty decay to a low value so as to avoid retaining the dirty pages for long (this ensures that pages are moved to muzzy sooner)
        int64_t low_dirty_decay_ms = 1;
        result = gballoc_ll_set_option("dirty_decay", &low_dirty_decay_ms);
        ASSERT_ARE_EQUAL(int, 0, result);
        result = gballoc_ll_set_option("muzzy_decay", &decay_ms);
    }
    ASSERT_ARE_EQUAL(int, 0, result);

    // free all the allocations
    for (uint32_t i = 0; i < num_allocations; i++)
    {
        gballoc_ll_free(ptr[i]);
    }
    gballoc_ll_free(ptr);

    // advance the epoch to update stats
    size_t epoch = 1;
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("epoch", NULL, NULL, &epoch, sizeof(epoch)));

    char command[64];
    size_t num_dirty;
    size_t num_muzzy;
    size_t num_dirty_total_before_sleep = 0;
    size_t num_muzzy_total_before_sleep = 0;
    size_t sizet_size = sizeof(num_dirty);
    if (is_dirty)
    {
        for (uint32_t i = 0; i < expected_narenas; i++)
        {
            snprintf(command, sizeof(command), "stats.arenas.%" PRIu32 ".pdirty", i);
            result = je_mallctl(command, (void *)&num_dirty, &sizet_size, NULL,0);
            if (result == 0)
            {
                LogInfo("Number of dirty pages for arena %d = %zu", i, num_dirty);
                num_dirty_total_before_sleep += num_dirty;
            }
            else
            {
                // Arenas may not have been created yet
                LogInfo("Fetching number of dirty pages failed for arena %d as it may not have been created yet", i);
            }
        }
    }
    else
    {
        for (uint32_t i = 0; i < expected_narenas; i++)
        {
            snprintf(command, sizeof(command), "stats.arenas.%" PRIu32 ".pmuzzy", i);
            result = je_mallctl(command, (void *)&num_muzzy, &sizet_size, NULL,0);
            if (result == 0)
            {
                LogInfo("Number of muzzy pages for arena %d = %zu", i, num_muzzy);
                num_muzzy_total_before_sleep += num_muzzy;
            }
            else
            {
                // Arenas may not have been created yet
                LogInfo("Fetching number of muzzy pages failed for arena %d as it may not have been created yet", i);
            }
        }
    }

    // sleep for a while to allow the decay to happen
    ThreadAPI_Sleep(sleep_time_ms);

    // force decay for all arenas, this will free all the dirty or muzzy pages that have been retained for longer than the decay time
    for (uint32_t i = 0; i < expected_narenas; i++)
    {
        snprintf(command, sizeof(command), "arena.%" PRIu32 ".decay", i);
        ASSERT_ARE_EQUAL(int, 0, je_mallctl(command, NULL, NULL, NULL, 0));
    }

    // advance the epoch to update stats
    epoch += 1;
    ASSERT_ARE_EQUAL(int, 0, je_mallctl("epoch", NULL, NULL, &epoch, sizeof(epoch)));

    size_t num_dirty_total_after_sleep = 0;
    size_t num_muzzy_total_after_sleep = 0;
    if (is_dirty)
    {
        for (uint32_t i = 0; i < expected_narenas; i++)
        {
            snprintf(command, sizeof(command), "stats.arenas.%" PRIu32 ".pdirty", i);
            result = je_mallctl(command, (void *)&num_dirty, &sizet_size, NULL,0);
            if (result == 0)
            {
                LogInfo("Number of dirty pages for arena %d = %zu", i, num_dirty);
                num_dirty_total_after_sleep += num_dirty;
            }
            else
            {
                // Arenas may not have been created yet
                LogInfo("Fetching number of dirty pages failed for arena %d as it may not have been created yet", i);
            }
        }

        if (is_percent_max)
        {
            // the number of dirty pages should be less than the residual dirty pages percent
            ASSERT_IS_TRUE(num_dirty_total_after_sleep <= (num_dirty_total_before_sleep * residual_pages_percent / 100));
        }
        else
        {
            // the number of dirty pages should be more than the residual dirty pages percent
            ASSERT_IS_TRUE(num_dirty_total_after_sleep >= (num_dirty_total_before_sleep * residual_pages_percent / 100));
        }
    }
    else
    {
        for (uint32_t i = 0; i < expected_narenas; i++)
        {
            snprintf(command, sizeof(command), "stats.arenas.%" PRIu32 ".pmuzzy", i);
            result = je_mallctl(command, (void *)&num_muzzy, &sizet_size, NULL,0);
            if (result == 0)
            {
                LogInfo("Number of muzzy pages for arena %d = %zu", i, num_muzzy);
                num_muzzy_total_after_sleep += num_muzzy;
            }
            else
            {
                // Arenas may not have been created yet
                LogInfo("Fetching number of muzzy pages failed for arena %d as it may not have been created yet", i);
            }
        }

        if (is_percent_max)
        {
            // the number of muzzy pages should be less than the residual dirty pages percent
            ASSERT_IS_TRUE(num_muzzy_total_after_sleep <= (num_muzzy_total_before_sleep * residual_pages_percent / 100));
        }
        else
        {
            // the number of muzzy pages should be more than the residual dirty pages percent
            ASSERT_IS_TRUE(num_muzzy_total_after_sleep >= (num_muzzy_total_before_sleep * residual_pages_percent / 100));
        }
    }
}

TEST_FUNCTION(gballoc_ll_set_option_check_dirty_pages_with_decay_5_second)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 5000;
    uint32_t residual_dirty_pages_percent = 10;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(num_allocations, alloc_size, decay_ms, expected_narenas, residual_dirty_pages_percent, true, sleep_time_ms, true);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_dirty_pages_with_decay_5_minutes)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 300000;
    uint32_t residual_dirty_pages_percent = 90;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(num_allocations, alloc_size, decay_ms, expected_narenas, residual_dirty_pages_percent, false, sleep_time_ms, true);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_dirty_pages_with_decay_minus_one)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = -1;
    uint32_t residual_dirty_pages_percent = 95;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(num_allocations, alloc_size, decay_ms, expected_narenas, residual_dirty_pages_percent, false, sleep_time_ms, true);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_muzzy_pages_with_decay_5_seconds)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = 5000;
    uint32_t residual_muzzy_pages_percent = 10;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(num_allocations, alloc_size, decay_ms, expected_narenas, residual_muzzy_pages_percent, true, sleep_time_ms, false);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_muzzy_pages_with_decay_5_minutes)
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
    gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(num_allocations, alloc_size, decay_ms, expected_narenas, residual_muzzy_pages_percent, false, sleep_time_ms, false);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

TEST_FUNCTION(gballoc_ll_set_option_check_muzzy_pages_with_decay_minus_one)
{
    /// arrange
    uint32_t expected_narenas = MAX_ARENAS;

    // we will do 2 GB of allocations
    uint32_t num_allocations = 2000000;
    size_t alloc_size = 1024;

    int64_t decay_ms = -1;
    uint32_t residual_muzzy_pages_percent = 95;

    // sleep for 30 seconds
    uint32_t sleep_time_ms = 30000;

    ///act
    ///assert
    gballoc_ll_set_option_decay_check_dirty_or_muzzy_pages(num_allocations, alloc_size, decay_ms, expected_narenas, residual_muzzy_pages_percent, false, sleep_time_ms, false);

    ///clean
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("dirty_decay", &default_dirty_decay_ms));
    ASSERT_ARE_EQUAL(int, 0, gballoc_ll_set_option("muzzy_decay", &default_muzzy_decay_ms));
}

END_TEST_SUITE(TEST_SUITE_NAME_FROM_CMAKE)
